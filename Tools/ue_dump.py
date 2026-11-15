# UE 에디터 안에서 도는 스크립트. 에셋을 텍스트(T3D)로 뽑아 Saved/AssetDump 에 쓴다.
# 직접 실행하지 않고 Tools/ue.py 로 돌린다:
#   py Tools/ue.py Tools/ue_dump.py /Game/Storm /Game/Assets/WAU/Anims/AM_Player_Roll
# 인자는 폴더(/Game/Storm)나 에셋 경로. 없으면 /Game/Storm.
# T3D에는 기본값과 다른 프로퍼티만 나온다. 블루프린트는 그래프 노드와 클래스 기본값(CDO)을 따로 쓴다.
import os
import sys
import traceback

import unreal

OUT_ROOT = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "AssetDump")
SKIP_CLASSES = {"World", "MapBuildDataRegistry", "ObjectRedirector"}


def make_exporter():
    cls = unreal.load_class(None, "/Script/UnrealEd.ObjectExporterT3D")
    return unreal.new_object(cls)


def export_t3d(obj, out_file):
    os.makedirs(os.path.dirname(out_file), exist_ok=True)
    task = unreal.AssetExportTask()
    task.set_editor_property("object", obj)
    task.set_editor_property("filename", out_file)
    task.set_editor_property("exporter", make_exporter())
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    return unreal.Exporter.run_asset_export_task(task)


def collect(targets):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Game"], True)
    found = {}
    for target in targets:
        datas = registry.get_assets_by_path(target, recursive=True)
        if not datas:
            datas = registry.get_assets_by_package_name(target.split(".")[0])
        if not datas:
            unreal.log_warning("ue_dump: 에셋을 못 찾음 " + target)
        for data in datas:
            # 블루프린트 패키지에는 Blueprint 와 BP_X_C(GeneratedClass) 가 같이 있다. Blueprint 쪽만 쓴다.
            if str(data.asset_class_path.asset_name).endswith("GeneratedClass"):
                continue
            found[str(data.package_name)] = data
    return [found[key] for key in sorted(found)]


def dump(data):
    package = str(data.package_name)
    class_name = str(data.asset_class_path.asset_name)
    if class_name in SKIP_CLASSES:
        return None
    asset = data.get_asset()
    if asset is None:
        return "%s (%s): 로드 실패" % (package, class_name)

    base = os.path.join(OUT_ROOT, package.strip("/").replace("/", os.sep))
    ok = export_t3d(asset, base + ".t3d")
    line = "%s (%s): %s" % (package, class_name, "ok" if ok else "T3D 실패")

    if isinstance(asset, unreal.Blueprint):
        generated = asset.generated_class()
        if generated is not None:
            cdo = unreal.get_default_object(generated)
            ok_cdo = export_t3d(cdo, base + ".defaults.t3d")
            line += ", defaults %s" % ("ok" if ok_cdo else "실패")
    return line


def main():
    targets = sys.argv[1:] or ["/Game/Storm"]
    lines = []
    for data in collect(targets):
        try:
            line = dump(data)
        except Exception:
            line = "%s: 예외\n%s" % (data.package_name, traceback.format_exc())
        if line:
            lines.append(line)
            unreal.log("ue_dump: " + line)

    os.makedirs(OUT_ROOT, exist_ok=True)
    with open(os.path.join(OUT_ROOT, "index.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    unreal.log("ue_dump: %d개 → %s" % (len(lines), OUT_ROOT))


main()
