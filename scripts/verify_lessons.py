"""Build the teaching projects and check their documented behaviors (Python 3.9+)."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-root', type=Path, help='Scratch directory for builds and logs')
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1]
    output = (args.build_root or source / 'build/lesson-verification').resolve()
    output.mkdir(parents=True, exist_ok=True)
    results = []

    def run(label, command, expected=0, contains=None):
        print(label, flush=True)
        process = subprocess.run([str(x) for x in command], stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, encoding='utf-8', errors='replace',
                                 timeout=240, check=False)
        (output / (label + '.txt')).write_text(process.stdout, encoding='utf-8')
        ok = process.returncode == expected if expected == 0 else process.returncode != 0
        if contains is not None:
            ok = ok and contains in process.stdout
        results.append({'name': label, 'passed': ok, 'exit_code': process.returncode,
                        'expected_failure': expected != 0})
        (output / 'results.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
        if not ok:
            raise RuntimeError(f'{label} failed:\n{process.stdout}')
        return process.stdout

    def configure(name, directory, config, *options, generator='Ninja'):
        destination = output / name
        command = ['cmake', '-S', source / directory, '-B', destination, '-G', generator]
        if generator == 'Ninja':
            command.append('-DCMAKE_BUILD_TYPE=' + config)
        command.extend(options)
        run(name + '-configure', command)
        return destination

    def build(name, directory, config):
        run(name + '-build', ['cmake', '--build', directory, '--config', config, '--parallel', '2'])

    def test(name, directory, config, expected=0, contains=None):
        return run(name + '-test', ['ctest', '--test-dir', directory, '-C', config,
                                    '--output-on-failure', '--no-tests=error'], expected, contains)

    def executable(directory, name):
        return directory / (name + ('.exe' if os.name == 'nt' else ''))

    run('cmake-version', ['cmake', '--version'])
    run('ninja-version', ['ninja', '--version'])
    run('variable-scope', ['cmake', '-P', source / 'lessons/cmake/variables.cmake'],
        contains='outside label=outer')
    for config in ('Debug', 'Release'):
        suffix = config.lower()
        name = 'foundations-' + suffix
        directory = configure(name, 'lessons/foundations', config)
        build(name, directory, config)
        test(name, directory, config, contains='12/12')
        trace = run(name + '-lifetime', [executable(directory, 'basic_08_lifetime')])
        expected_trace = ['open outer', 'open resource', 'owner moved', 'close resource',
                          'leaving main', 'close outer']
        if trace.splitlines() != expected_trace:
            raise RuntimeError('RAII destruction order differs from the lesson')

        name = 'hello-' + suffix
        directory = configure(name, 'lessons/cmake/01_hello', config)
        build(name, directory, config)
        run(name + '-run', [executable(directory, 'hello')], contains='Hello, CMake!')

        name = 'library-' + suffix
        directory = configure(name, 'lessons/cmake/02_library', config)
        build(name, directory, config)
        test(name, directory, config)

        name = 'taskkit-' + suffix
        directory = configure(name, 'lessons/cmake/03_project', config)
        build(name, directory, config)
        test(name, directory, config)
        run(name + '-run', [executable(directory / 'app', 'taskkit_app')], contains='TaskKit 1.0.0\ntrue')
        prefix = output / ('install-' + suffix)
        run(name + '-install', ['cmake', '--install', directory, '--config', config, '--prefix', prefix])

        # Two independent consumer builds: original install tree and copied install tree.
        for mode in ('original', 'relocated'):
            consumer_prefix = prefix
            if mode == 'relocated':
                consumer_prefix = output / ('relocated-' + suffix)
                shutil.copytree(prefix, consumer_prefix, dirs_exist_ok=True)
            name = 'consumer-' + mode + '-' + suffix
            directory = configure(name, 'lessons/cmake/04_consumer', config,
                                  '-DCMAKE_PREFIX_PATH=' + str(consumer_prefix))
            build(name, directory, config)
            test(name, directory, config)
            cache = (directory / 'CMakeCache.txt').read_text(encoding='utf-8')
            package_dir = next(line.split('=', 1)[1] for line in cache.splitlines()
                               if line.startswith('TaskKit_DIR:'))
            Path(package_dir).resolve().relative_to(consumer_prefix.resolve())

    name = 'taskkit-multiconfig'
    directory = configure(name, 'lessons/cmake/03_project', 'Debug', generator='Ninja Multi-Config')
    build(name, directory, 'Debug')
    test(name, directory, 'Debug')

    name = 'taskkit-library-only'
    directory = configure(name, 'lessons/cmake/03_project', 'Release',
                          '-DTASKKIT_BUILD_APP=OFF', '-DBUILD_TESTING=OFF')
    build(name, directory, 'Release')
    test(name, directory, 'Release', expected=1, contains='No tests were found')

    # Mutate ONLY an isolated copy to prove the boundary test really detects a bug.
    fault_source = output / 'fault-source'
    shutil.copytree(source / 'lessons/cmake/03_project', fault_source, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('build', '.git', 'CMakeUserPresets.json'))
    cpp = fault_source / 'src/title.cpp'
    original = cpp.read_text(encoding='utf-8')
    if 'text.size() > 200' not in original:
        raise RuntimeError('Boundary mutation no longer matches the example')
    cpp.write_text(original.replace('text.size() > 200', 'text.size() > 199'), encoding='utf-8')
    fault_build = output / 'fault-build'
    run('negative-boundary-configure', ['cmake', '-S', fault_source, '-B', fault_build,
                                      '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release'])
    build('negative-boundary', fault_build, 'Release')
    test('negative-boundary', fault_build, 'Release', expected=1, contains='FAIL: 200-byte boundary')
    print(f'PASS: {len(results)} verification steps; logs: {output}')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, ValueError, StopIteration, subprocess.TimeoutExpired) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
