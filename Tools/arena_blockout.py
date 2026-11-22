# 컨셉 아트(Docs 참고)대로 아레나 블록아웃을 /Game/Storm/Arena 에 배치한다. 열린 에디터로 원격 실행한다.
#   py Tools\ue.py Tools\arena_blockout.py
# 다시 돌리면 Arena 폴더의 액터를 지우고 새로 놓는다. 저장은 하지 않는다 (에디터에서 직접 저장).
# 좌표: 원판 가운데가 (0, 0), 바닥 윗면 Z = 100. 진입로는 205도 방향에서 들어와 25도 방향(아레나 가운데)을 본다.
# 해는 DirectionalLight(yaw 160)라 -20도 방위에 있다. 그래서 아치 폐허를 -20도에 둬서 진입하면 해가 아치 뒤로 진다.
import math
import random
import unreal

LEVEL = "/Game/Storm/Arena"
FOLDER = "Arena"
TOP = 100.0
RADIUS = 2000.0
DEPTH = 30000.0
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
PILLAR = "/Game/GriidArt_Free_Pillar/Static_Meshes/SM_GriidArt_PillarFull"
BROKEN_A = "/Game/GriidArt_Free_Pillar/Static_Meshes/SM_GriidArt_PillarBroken_A"
BROKEN_B = "/Game/GriidArt_Free_Pillar/Static_Meshes/SM_GriidArt_PillarBroken_B"
LINTEL = "/Game/GriidArt_Free_Pillar/Static_Meshes/SM_GriidArt_Head"
DEBRIS = "/Game/GriidArt_Free_Pillar/Static_Meshes/SM_GriidArt_Pillar_Debris"
# Megascans 절벽 (Fab, Medium 품질). 피벗과 크기가 제각각이라 배치할 때 바운드로 맞춘다.
# Nordic 4종은 드론으로 위에서 찍은 해안 바위라 스캔된 면이 +Z(윗면)를 보고 밑면이 비어 있다 (평균 노멀로 확인).
# 그래서 +Z가 원판 바깥을 보게 눕혀 세운다. Sharp Cliff는 스캔 면이 +Y를 본다
MS = "/Game/Fab/Megascans/3D/"
CLIFF_HUGE = MS + "Huge_Nordic_Coastal_Cliff_venrdcgga/Medium/venrdcgga_tier_2/StaticMeshes/venrdcgga_tier_2"
CLIFF_MASSIVE_A = MS + "Massive_Nordic_Coastal_Cliff_vdssailfa/Medium/vdssailfa_tier_2/StaticMeshes/vdssailfa_tier_2"
CLIFF_MASSIVE_B = MS + "Massive_Nordic_Coastal_Cliff_vfmjadufa/Medium/vfmjadufa_tier_2/StaticMeshes/vfmjadufa_tier_2"
CLIFF_GIANT = MS + "Massive_Nordic_Coastal_Cliff_veqnci0db/Medium/veqnci0db_tier_2/StaticMeshes/veqnci0db_tier_2"
CLIFF_FACE = MS + "Sharp_Cliff_rcygn/Medium/rcygn_tier_2/StaticMeshes/rcygn_tier_2"
# 진입로 옆면 절벽(Sharp Cliff)은 뒷면이 없는 메시라, 화면에서 뒷면이 보이면 True로 바꿔 돌려 세운다
FLIP_FACE = False
ROCK_MATERIAL = "/Game/Storm/Environment/Materials/MI_ArenaRock"
MOUNTAINS = [
    "/Game/Assets/mountain_1/StaticMeshes/mountain_Material_002_0",
    "/Game/Assets/mountain_1/StaticMeshes/mountain_Material_002_0_651c26041deea911a295160691fd9aa7",
    "/Game/Assets/mountain_1/StaticMeshes/mountain_Material_002_0_680afa8ccbb5433a316aef545d1054ec",
    "/Game/Assets/mountain_1/StaticMeshes/mountain_Material_002_0_9afff822ad8bfd41d2aedc9edd502c6c",
]
ENTRY_ANGLE = 205.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def check_level():
    # 맵을 Python에서 복사하고 바로 열면 복사본 월드가 GC되지 않아 에디터가 죽는다 (11/20, World Memory Leaks).
    # 그래서 레벨 만들기는 에디터에서 직접 한다: Test 레벨을 열고 File > Save Current Level As 로 /Game/Storm/Arena
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ok = world.get_path_name().startswith(LEVEL + ".")
    if not ok:
        print("지금 레벨이 %s 라서 멈춤. %s 를 열고 다시 돌린다" % (world.get_path_name(), LEVEL))
    return ok


def polar(angle_deg, r):
    a = math.radians(angle_deg)
    return unreal.Vector(math.cos(a) * r, math.sin(a) * r, 0.0)


def place(label, mesh, location, scale, yaw=0.0):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, location, unreal.Rotator(0.0, 0.0, yaw))
    actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    actor.set_actor_scale3d(scale)
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset(mesh))
    return actor


def block(label, mesh, center_xy, size, yaw=0.0, base=TOP):
    # size는 cm (가로, 세로, 높이). 기본 도형은 100cm라 크기 / 100이 스케일. 밑면을 base에 맞춘다
    loc = unreal.Vector(center_xy.x, center_xy.y, base + size[2] * 0.5)
    return place(label, mesh, loc, unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0), yaw)


def ruin(label, angle, r, parts):
    # parts: (메시, 접선 방향 오프셋, 반지름 방향 오프셋, 바닥에서 높이, 회전 더하기). 접선 방향은 가장자리를 따라간다.
    # 폐허 메시는 GriidArt 기둥 팩이고 피벗이 밑면이라 스케일 1로 바닥(TOP)에 세운다
    center = polar(angle, r)
    tangent = polar(angle + 90.0, 1.0)
    radial = polar(angle, 1.0)
    for i, (mesh, t, rr, z, yaw) in enumerate(parts):
        p = center + tangent * t + radial * rr
        place("%s_%d" % (label, i), mesh, unreal.Vector(p.x, p.y, TOP + z), unreal.Vector(1, 1, 1), angle + 90.0 + yaw)


def cliff(label, mesh_path, angle, radial_in, top, scale, roll, radius=RADIUS):
    # Nordic 절벽을 원판 둘레에 세운다. 메시 +Z(스캔 면)가 반지름 바깥, +X가 가장자리를 따라, +Y가 위를 본다.
    # radial_in은 메시 두께(로컬 Z) 중 원기둥 안에 묻힐 비율. 윗면을 top에 맞추고, roll로 바깥 축을 중심으로 조금 기울인다.
    # 충돌은 끈다 (가장자리 밖 턱에 걸려 낙사가 막히지 않게)
    mesh = unreal.load_asset(mesh_path)
    b = mesh.get_bounds()
    out = polar(angle, 1.0)
    along = polar(angle + 90.0, 1.0)
    rot = unreal.MathLibrary.make_rot_from_xz(along, out)
    # 로컬 Z(바깥 축) 둘레로 먼저 돌린 뒤 세운다
    rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(0.0, 0.0, roll), rot)
    half_out = b.box_extent.z * scale
    half_up = b.box_extent.y * scale
    center = polar(angle, radius + half_out * (1.0 - 2.0 * radial_in))
    center.z = top - half_up
    # 바운드 가운데의 피벗 기준 오프셋을 회전해서 뺀다
    off = unreal.MathLibrary.greater_greater_vector_rotator(unreal.Vector(b.origin.x * scale, b.origin.y * scale, b.origin.z * scale), rot)
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, center - off, rot)
    actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_collision_profile_name("NoCollision")
    # 기울이면 위아래 폭이 늘어 바닥 위로 솟을 수 있다. 실제 바운드 윗면을 top에 다시 맞춘다 (충돌이 없어 솟은 바위는 통과된다)
    o, e = actor.get_actor_bounds(False)
    loc = actor.get_actor_location()
    actor.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + top - (o.z + e.z)), False, False)
    return actor


def dress_cliffs():
    if not unreal.EditorAssetLibrary.does_asset_exist(CLIFF_HUGE):
        print("Megascans 절벽이 없어 절벽 장식을 건너뜀")
        return
    rng = random.Random(7)

    def near_entry(angle):
        d = (angle - ENTRY_ANGLE + 180.0) % 360.0 - 180.0
        return abs(d) < 14.0

    # 윗단: 원판 가장자리 바로 아래를 둘러 원기둥 옆면을 가린다. 윗면은 바닥보다 조금 낮게 둬서 걸리지 않게
    upper = [CLIFF_MASSIVE_A, CLIFF_MASSIVE_B, CLIFF_HUGE, CLIFF_MASSIVE_B, CLIFF_MASSIVE_A, CLIFF_HUGE]
    count = 12
    for i in range(count):
        angle = i * 360.0 / count + rng.uniform(-5, 5)
        if near_entry(angle):
            continue
        mesh = upper[i % len(upper)]
        # 1배로는 위아래 8~15m라 바위가 가장자리에 혹처럼 붙고 그 아래 원기둥이 드러났다 (11/20 스크린샷). 키워서 20~30m를 덮는다
        scale = rng.uniform(1.9, 2.3) if mesh == CLIFF_HUGE else rng.uniform(1.4, 1.7)
        # 두께의 65%를 원기둥 안에 묻는다. 많이 튀어나오면 가장자리 밖에 턱처럼 보이는데 충돌이 없어 빠진다
        cliff("Cliff_Upper_%d" % i, mesh, angle, 0.65, TOP - rng.uniform(40, 140), scale, rng.uniform(-12, 12))

    # 아랫단: 큰 덩어리로 절벽 아래쪽을 받친다
    for i in range(9):
        angle = i * 360.0 / 9 + 20.0 + rng.uniform(-8, 8)
        mesh = CLIFF_GIANT if i % 2 == 0 else CLIFF_MASSIVE_A
        scale = rng.uniform(1.0, 1.25) if mesh == CLIFF_GIANT else rng.uniform(1.8, 2.2)
        # 떨어지는 캐릭터가 바위를 뚫고 지나가는 게 보이지 않게 아랫단도 원기둥에 붙인다
        cliff("Cliff_Lower_%d" % i, mesh, angle, 0.7, TOP - rng.uniform(1500, 2100), scale, rng.uniform(-15, 15))

    # 진입로 양옆: 세로로 갈라진 벽면. 메시 X축을 진입로 방향에 맞춘다
    along = polar(ENTRY_ANGLE, 1.0)
    side = polar(ENTRY_ANGLE + 90.0, 1.0)
    face = unreal.load_asset(CLIFF_FACE)
    fb = face.get_bounds()
    for s_i, sign in enumerate((1.0, -1.0)):
        for k, dist in enumerate((RADIUS + 600.0, RADIUS + 1500.0)):
            yaw = ENTRY_ANGLE + (0.0 if (sign > 0) != FLIP_FACE else 180.0)
            center = along * dist + side * (sign * (400.0 + fb.box_extent.y * 0.3))
            a = math.radians(yaw)
            off = unreal.Vector(fb.origin.x * math.cos(a) - fb.origin.y * math.sin(a), fb.origin.x * math.sin(a) + fb.origin.y * math.cos(a), 0)
            z = TOP - 30.0 - (fb.origin.z + fb.box_extent.z)
            act = place("Cliff_Entry_%d_%d" % (s_i, k), CLIFF_FACE, unreal.Vector(center.x - off.x, center.y - off.y, z), unreal.Vector(1, 1, 1), yaw)
            act.static_mesh_component.set_collision_profile_name("NoCollision")
    # 진입로 끝을 바위로 막는다
    cliff("Cliff_EntryEnd", CLIFF_HUGE, ENTRY_ANGLE, 0.5, TOP - 40.0, 1.3, 0.0, radius=RADIUS + 2100.0)


def clear_folder():
    for a in actors.get_all_level_actors():
        if str(a.get_folder_path()).startswith(FOLDER):
            actors.destroy_actor(a)


def find_actor(cls):
    for a in actors.get_all_level_actors():
        if isinstance(a, cls):
            return a
    return None


def main():
    if not check_level():
        return
    clear_folder()

    # 테스트용 바닥, 그림자 실험 기둥, 허수아비는 지운다
    for a in actors.get_all_level_actors():
        label = a.get_actor_label()
        if label in ("Floor", "Cube") or a.get_class().get_name().startswith("BP_StormTargetDummy"):
            actors.destroy_actor(a)
            print("지움", label)

    # 원판과 절벽: 원기둥 하나. 윗면이 TOP
    place("Plateau", CYLINDER, unreal.Vector(0, 0, TOP - DEPTH * 0.5), unreal.Vector(RADIUS * 2 / 100.0, RADIUS * 2 / 100.0, DEPTH / 100.0))

    # 진입로: 205도에서 들어오는 좁은 능선. 가운데 쪽 끝이 원판 안으로 조금 겹친다
    entry_len = 2200.0
    entry_center = polar(ENTRY_ANGLE, RADIUS + entry_len * 0.5 - 300.0)
    place("Entry", CUBE, unreal.Vector(entry_center.x, entry_center.y, TOP - DEPTH * 0.5),
          unreal.Vector(entry_len / 100.0, 800.0 / 100.0, DEPTH / 100.0), ENTRY_ANGLE)

    # 바위 재질 (Tools/arena_materials.py 로 만든다)
    if unreal.EditorAssetLibrary.does_asset_exist(ROCK_MATERIAL):
        rock = unreal.load_asset(ROCK_MATERIAL)
        for a in actors.get_all_level_actors():
            if a.get_actor_label() in ("Plateau", "Entry"):
                a.static_mesh_component.set_material(0, rock)

    # 폐허 4곳 (가장자리만, 가운데는 비운다). 기둥은 높이 5.1m, 들보는 길이 5m라 기둥 중심을 193 띄운다
    R = RADIUS - 250.0
    ruin("Ruin_Arch", -20.0, R, [
        (PILLAR, -193.0, 0.0, 0.0, 0.0),
        (PILLAR, 193.0, 0.0, 0.0, 0.0),
        (LINTEL, 0.0, 0.0, 512.0, 0.0),
        (BROKEN_A, -560.0, 40.0, 0.0, 30.0),
        (DEBRIS, 420.0, 60.0, 0.0, 70.0),
    ])
    ruin("Ruin_Tower", 70.0, R, [
        (BROKEN_A, 0.0, 0.0, 0.0, 0.0),
        (BROKEN_B, 200.0, -60.0, 0.0, 45.0),
        (DEBRIS, -200.0, 20.0, 0.0, 10.0),
    ])
    ruin("Ruin_Small", -95.0, R, [
        (BROKEN_B, 0.0, 0.0, 0.0, 0.0),
        (DEBRIS, 150.0, 0.0, 0.0, 120.0),
    ])
    ruin("Ruin_Column", 135.0, R, [
        (PILLAR, 0.0, 0.0, 0.0, 15.0),
        (DEBRIS, 120.0, -50.0, 0.0, 200.0),
    ])

    dress_cliffs()

    # 아래 골짜기: 산 지형 타일 4개를 같은 자리에 둔다 (타일끼리 피벗이 같다)
    for i, m in enumerate(MOUNTAINS):
        valley = place("Valley_%d" % i, m, unreal.Vector(0, 0, -DEPTH), unreal.Vector(1, 1, 1))
        # 이 메시들의 단순 충돌은 메시를 감싸는 상자라서, 상자 윗면이 원판보다 높게 솟아 보이지 않는 천장이 된다 (플레이어가 Z 2466에서 시작함). 배경이라 충돌을 끈다
        valley.static_mesh_component.set_collision_profile_name("NoCollision")

    # 플레이어 시작: 진입로 위, 아레나 가운데를 본다
    start = find_actor(unreal.PlayerStart)
    if start:
        p = polar(ENTRY_ANGLE, RADIUS + 700.0)
        start.set_actor_location_and_rotation(unreal.Vector(p.x, p.y, TOP + 90.0), unreal.Rotator(0.0, 0.0, ENTRY_ANGLE - 180.0), False, False)

    # 보스: 아레나 가운데, 진입로를 본다 (낙뢰 컴포넌트가 보스 시작 위치를 아레나 가운데로 쓴다)
    for a in actors.get_all_level_actors():
        if a.get_class().get_name().startswith("BP_Boss1"):
            a.set_actor_location_and_rotation(unreal.Vector(0, 0, TOP + 150.0), unreal.Rotator(0.0, 0.0, ENTRY_ANGLE), False, False)

    # 내비 메시: 원판과 진입로만 덮는다
    nav = find_actor(unreal.NavMeshBoundsVolume)
    if nav:
        nav.set_actor_location(unreal.Vector(0, 0, TOP), False, False)
        nav.set_actor_scale3d(unreal.Vector(45.0, 45.0, 3.0))

    # 산이 원판 근처에서 너무 높이 솟았는지 본다
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    highest = -1e9
    for x in range(-6000, 6001, 1000):
        for y in range(-6000, 6001, 1000):
            if math.hypot(x, y) < RADIUS + 200:
                continue
            hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, TOP + 50000), unreal.Vector(x, y, -60000),
                                                         unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
            if hit:
                t = hit.to_tuple()
                if t[0] or t[1]:
                    highest = max(highest, t[4].z)
    print("원판 주변 6km 안 가장 높은 지형 Z", highest)
    print("배치 끝. 에디터에서 저장하세요 (Ctrl+Shift+S)")


main()
