# 아레나 바위 재질을 만든다. 열린 에디터로 원격 실행한다.
#   py Tools\ue.py Tools\arena_materials.py
# 1. RawAssets/PolyHaven 의 텍스처를 /Game/Storm/Environment/Textures 로 임포트한다 (이미 있으면 건너뛴다)
# 2. M_ArenaRock: 월드 좌표로 투영하는 트라이플래너 재질. 윗면(Z)은 Top 텍스처, 옆면(X, Y)은 Side 텍스처.
#    원기둥과 상자를 크게 늘린 블록이라 메시 UV를 쓰면 늘어나서, 크기와 상관없이 같은 밀도로 반복되게 월드 좌표를 쓴다.
#    노멀은 투영 축마다 월드 공간으로 돌려 섞고, 재질은 월드 공간 노멀을 출력한다 (Tangent Space Normal 끔)
# 3. MI_ArenaRock 을 만들어 아레나의 Plateau 와 Entry 에 입힌다
# 다시 돌리면 재질 그래프를 새로 만든다. 끝나면 만든 에셋을 저장한다 (레벨은 저장하지 않음)
import os
import unreal

PROJECT = unreal.SystemLibrary.get_project_directory()
RAW = os.path.join(PROJECT, "RawAssets", "PolyHaven")
TEX_DIR = "/Game/Storm/Environment/Textures"
MAT_DIR = "/Game/Storm/Environment/Materials"

TEXTURES = {
    # 에셋 이름: (파일, 종류)
    "T_MonasteryFloor_D": ("monastery_stone_floor_diff_2k.jpg", "color"),
    "T_MonasteryFloor_N": ("monastery_stone_floor_nor_dx_2k.jpg", "normal"),
    "T_MonasteryFloor_ARM": ("monastery_stone_floor_arm_2k.jpg", "mask"),
    "T_RockFace04_D": ("rock_face_04_diff_2k.jpg", "color"),
    "T_RockFace04_N": ("rock_face_04_nor_dx_2k.jpg", "normal"),
    "T_RockFace04_ARM": ("rock_face_04_arm_2k.jpg", "mask"),
}

# 축마다 가중치. Sharp가 클수록 투영이 바뀌는 경계가 좁다.
# 윗면(바닥)은 같은 텍스처가 TopTile마다 그대로 반복돼 무늬가 보여서 두 가지로 깬다 (11/20 사용자 지적).
#  1. 회전하고 크기를 바꾼 두 번째 샘플을 값 노이즈(VarScale 크기의 얼룩)로 섞는다
#  2. 더 큰 노이즈(MacroScale)로 밝기를 얼룩지게 한다 (MacroStrength, ARM에서는 0)
# 노이즈는 텍스처 없이 격자 해시로 만든다
WEIGHTS = """
float3 w = pow(abs(normalize(N)), Sharp);
w /= max(w.x + w.y + w.z, 1e-4);
float2 uvTop = P.xy / TopTile;
float2 uvX = P.yz / SideTile;
float2 uvY = P.xz / SideTile;
const float2 K = float2(127.1, 311.7);
float2 q = P.xy / VarScale;
float2 qi = floor(q);
float2 qf = frac(q);
qf = qf * qf * (3 - 2 * qf);
float4 h = frac(sin(float4(dot(qi, K), dot(qi + float2(1, 0), K), dot(qi + float2(0, 1), K), dot(qi + float2(1, 1), K))) * 43758.5453);
float blend = smoothstep(0.3, 0.7, lerp(lerp(h.x, h.y, qf.x), lerp(h.z, h.w, qf.x), qf.y));
float2 m = P.xy / MacroScale;
float2 mi = floor(m);
float2 mf = frac(m);
mf = mf * mf * (3 - 2 * mf);
float4 g = frac(sin(float4(dot(mi, K), dot(mi + float2(1, 0), K), dot(mi + float2(0, 1), K), dot(mi + float2(1, 1), K))) * 43758.5453);
float macro = lerp(1 - MacroStrength, 1 + MacroStrength * 0.5, lerp(lerp(g.x, g.y, mf.x), lerp(g.z, g.w, mf.x), mf.y));
// 두 번째 샘플: 약 53도 돌리고 0.77배
const float ca = 0.6;
const float sa = 0.8;
float2 uvTopB = float2(ca * uvTop.x - sa * uvTop.y, sa * uvTop.x + ca * uvTop.y) * 0.77 + float2(0.37, 0.71);
"""

COLOR_CODE = WEIGHTS + """
float3 topA = Texture2DSample(TopTex, TopTexSampler, uvTop).rgb;
float3 topB = Texture2DSample(TopTex, TopTexSampler, uvTopB).rgb;
float3 top = lerp(topA, topB, blend) * macro;
float3 sx = Texture2DSample(SideTex, SideTexSampler, uvX).rgb;
float3 sy = Texture2DSample(SideTex, SideTexSampler, uvY).rgb;
return sx * w.x + sy * w.y + top * w.z;
"""

# 노멀 맵(BC5)은 rg만 있어서 z를 다시 만든다. 축마다 탄젠트 공간 노멀을 월드 축으로 돌린다.
# 돌린 두 번째 샘플의 노멀은 UV를 돌린 만큼 거꾸로 돌려서 섞는다
NORMAL_CODE = WEIGHTS + """
float3 n = normalize(N);
float2 a = Texture2DSample(SideTex, SideTexSampler, uvX).rg * 2 - 1;
float2 b = Texture2DSample(SideTex, SideTexSampler, uvY).rg * 2 - 1;
float2 cA = Texture2DSample(TopTex, TopTexSampler, uvTop).rg * 2 - 1;
float2 cB = Texture2DSample(TopTex, TopTexSampler, uvTopB).rg * 2 - 1;
cB = float2(ca * cB.x + sa * cB.y, -sa * cB.x + ca * cB.y);
float2 c = lerp(cA, cB, blend);
float3 tx = float3(a, sqrt(saturate(1 - dot(a, a))));
float3 ty = float3(b, sqrt(saturate(1 - dot(b, b))));
float3 tz = float3(c, sqrt(saturate(1 - dot(c, c))));
float3 nx = float3(tx.z * (n.x < 0 ? -1 : 1), tx.x, tx.y);
float3 ny = float3(ty.x, ty.z * (n.y < 0 ? -1 : 1), ty.y);
float3 nz = float3(tz.x, tz.y, tz.z * (n.z < 0 ? -1 : 1));
return normalize(nx * w.x + ny * w.y + nz * w.z);
"""

assets = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def import_textures():
    tasks = []
    for name, (file_name, _) in TEXTURES.items():
        if unreal.EditorAssetLibrary.does_asset_exist(TEX_DIR + "/" + name):
            continue
        task = unreal.AssetImportTask()
        task.filename = os.path.join(RAW, file_name)
        task.destination_path = TEX_DIR
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = False
        tasks.append(task)
    if tasks:
        assets.import_asset_tasks(tasks)
    for name, (_, kind) in TEXTURES.items():
        tex = unreal.load_asset(TEX_DIR + "/" + name)
        if kind == "normal":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif kind == "mask":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
    print("텍스처", len(tasks), "개 임포트")


def tex(name):
    return unreal.load_asset(TEX_DIR + "/" + name)


def make_material():
    path = MAT_DIR + "/M_ArenaRock"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.load_asset(path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = assets.create_asset("M_ArenaRock", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("tangent_space_normal", False)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def scalar(name, value, y):
        p = node(unreal.MaterialExpressionScalarParameter, -1200, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", value)
        return p

    def tex_param(name, texture, sampler, y):
        p = node(unreal.MaterialExpressionTextureObjectParameter, -1200, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("texture", texture)
        p.set_editor_property("sampler_type", sampler)
        return p

    pos = node(unreal.MaterialExpressionWorldPosition, -1200, -400)
    nrm = node(unreal.MaterialExpressionVertexNormalWS, -1200, -300)
    top_tile = scalar("TopTile", 180.0, -200)
    side_tile = scalar("SideTile", 400.0, -100)
    sharp = scalar("Sharp", 4.0, 0)
    var_scale = scalar("VarScale", 700.0, 100)
    macro_scale = scalar("MacroScale", 2500.0, 200)
    macro_strength = scalar("MacroStrength", 0.3, 300)
    no_macro = node(unreal.MaterialExpressionConstant, -1200, 400)

    def custom(desc, code, top, side, y, macro_input=None):
        c = node(unreal.MaterialExpressionCustom, -600, y)
        c.set_editor_property("description", desc)
        c.set_editor_property("code", code)
        c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        names = ["TopTex", "SideTex", "P", "N", "TopTile", "SideTile", "Sharp", "VarScale", "MacroScale", "MacroStrength"]
        inputs = []
        for n in names:
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", n)
            inputs.append(ci)
        c.set_editor_property("inputs", inputs)
        sources = ((top, "TopTex"), (side, "SideTex"), (pos, "P"), (nrm, "N"), (top_tile, "TopTile"), (side_tile, "SideTile"), (sharp, "Sharp"),
                   (var_scale, "VarScale"), (macro_scale, "MacroScale"), (macro_input or macro_strength, "MacroStrength"))
        for src, n in sources:
            mel.connect_material_expressions(src, "", c, n)
        return c

    S = unreal.MaterialSamplerType
    top_d = tex_param("TopDiffuse", tex("T_MonasteryFloor_D"), S.SAMPLERTYPE_COLOR, 200)
    side_d = tex_param("SideDiffuse", tex("T_RockFace04_D"), S.SAMPLERTYPE_COLOR, 350)
    top_n = tex_param("TopNormal", tex("T_MonasteryFloor_N"), S.SAMPLERTYPE_NORMAL, 500)
    side_n = tex_param("SideNormal", tex("T_RockFace04_N"), S.SAMPLERTYPE_NORMAL, 650)
    top_m = tex_param("TopARM", tex("T_MonasteryFloor_ARM"), S.SAMPLERTYPE_MASKS, 800)
    side_m = tex_param("SideARM", tex("T_RockFace04_ARM"), S.SAMPLERTYPE_MASKS, 950)

    color = custom("TriplanarColor", COLOR_CODE, top_d, side_d, 0)
    normal = custom("TriplanarNormal", NORMAL_CODE, top_n, side_n, 300)
    arm = custom("TriplanarARM", COLOR_CODE, top_m, side_m, 600, no_macro)

    # 바위 색을 어둡게 맞추는 곱
    tint = node(unreal.MaterialExpressionVectorParameter, -600, -250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    tinted = node(unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(color, "", tinted, "A")
    mel.connect_material_expressions(tint, "", tinted, "B")
    mel.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)

    # ARM: R가 AO, G가 거칠기
    ao = node(unreal.MaterialExpressionComponentMask, -300, 550)
    ao.set_editor_property("r", True)
    ao.set_editor_property("g", False)
    ao.set_editor_property("b", False)
    ao.set_editor_property("a", False)
    rough = node(unreal.MaterialExpressionComponentMask, -300, 650)
    rough.set_editor_property("r", False)
    rough.set_editor_property("g", True)
    rough.set_editor_property("b", False)
    rough.set_editor_property("a", False)
    mel.connect_material_expressions(arm, "", ao, "")
    mel.connect_material_expressions(arm, "", rough, "")
    mel.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    return mat


def make_instance(mat):
    path = MAT_DIR + "/MI_ArenaRock"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = assets.create_asset("MI_ArenaRock", MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    return mi


def apply(mi):
    for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if a.get_actor_label() in ("Plateau", "Entry"):
            a.static_mesh_component.set_material(0, mi)
            print("재질 적용", a.get_actor_label())


import_textures()
material = make_material()
instance = make_instance(material)
apply(instance)
unreal.EditorAssetLibrary.save_directory("/Game/Storm/Environment", only_if_is_dirty=True, recursive=True)
print("끝. 레벨은 에디터에서 저장하세요")
