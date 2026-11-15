# 에디터 밖에서 UE Python 스크립트를 돌리는 실행기.
#   py Tools/ue.py Tools/ue_dump.py /Game/Storm        열린 에디터로 보낸다 (원격 실행)
#   py Tools/ue.py --headless Tools/ue_dump.py ...     에디터 없이 커맨드릿으로 돌린다
#   py Tools/ue.py -c "import unreal; print(unreal.SystemLibrary.get_engine_version())"
# 원격 실행은 프로젝트 설정 > Plugins > Python > Enable Remote Execution 이 켜져 있어야 한다 (DefaultEngine.ini 에 넣어 둠).
# 에디터가 열려 있을 때 --headless 는 읽기 전용으로만 쓴다. 에셋 저장은 열린 에디터 쪽(원격)에서 한다.
import argparse
import os
import subprocess
import sys
import time

ENGINE = os.environ.get("UE_ROOT", r"C:\Program Files\Epic Games\UE_5.7")
PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_NAME = "EyeOfTheStorm"
PROJECT = os.path.join(PROJECT_DIR, PROJECT_NAME + ".uproject")


def run_remote(command, wait):
    sys.path.insert(0, os.path.join(ENGINE, r"Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"))
    import remote_execution as remote

    session = remote.RemoteExecution()
    session.start()
    try:
        deadline = time.time() + wait
        node = None
        while node is None and time.time() < deadline:
            for candidate in session.remote_nodes:
                if candidate.get("project_name") == PROJECT_NAME:
                    node = candidate
            time.sleep(0.1)
        if node is None:
            print("열린 에디터를 못 찾음. 에디터가 닫혀 있거나 원격 실행이 꺼져 있다. --headless 로 돌릴 수 있다.")
            return 2
        session.open_command_connection(node["node_id"])
        result = session.run_command(command, unattended=True, exec_mode=remote.MODE_EXEC_FILE)
    finally:
        session.stop()

    for entry in result.get("output", []):
        print("[%s] %s" % (entry.get("type"), entry.get("output", "").rstrip()))
    if not result.get("success"):
        print("실패: %s" % result.get("result"))
        return 1
    return 0


def run_headless(command):
    exe = os.path.join(ENGINE, r"Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
    line = '"%s" "%s" -run=pythonscript -script="%s" -unattended -nosplash -nullrhi -stdout -FullStdOutLogOutput' % (exe, PROJECT, command)
    proc = subprocess.run(line, capture_output=True)
    text = proc.stdout.decode("utf-8", errors="replace")
    for row in text.splitlines():
        if "LogPython" in row or "Error:" in row:
            print(row)
    return proc.returncode


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--headless", action="store_true")
    parser.add_argument("--wait", type=float, default=3.0)
    parser.add_argument("-c", dest="code")
    parser.add_argument("script", nargs="?")
    parser.add_argument("args", nargs=argparse.REMAINDER)
    options = parser.parse_args()

    if options.code:
        inline = os.path.join(PROJECT_DIR, "Saved", "AssetDump", "_inline.py")
        os.makedirs(os.path.dirname(inline), exist_ok=True)
        with open(inline, "w", encoding="utf-8") as f:
            f.write(options.code)
        command = inline.replace("\\", "/")
    elif options.script:
        # 역슬래시는 UE 명령줄 파서가 이스케이프로 먹는다 (\u 등)
        command = " ".join([os.path.abspath(options.script).replace("\\", "/")] + options.args)
    else:
        parser.error("스크립트 경로나 -c 코드가 필요하다")

    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.exit(run_headless(command) if options.headless else run_remote(command, options.wait))


main()
