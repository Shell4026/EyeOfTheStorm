# 보스가 죽을 때 쓰는 디졸브 재질을 만든다. 열린 에디터로 원격 실행한다.
#   py Tools\ue.py Tools\boss_death_materials.py
# 1. MF_DeathDissolve: 스킨 전 위치(PreSkinnedPosition)로 3D 노이즈를 만들어, DeathFade가 1에서 0으로 내려가는 동안
#    노이즈가 낮은 곳부터 구멍이 나고 구멍 가장자리가 빛난다. 위치를 스킨 전 좌표로 잡아 쓰러지는 동안 무늬가 몸에 붙어 있다
# 2. Sevarog 재질 5개를 /Game/Storm/Mob/Boss1/Materials 에 _Death 로 복사한다.
#    원본은 Content/ParagonSevarog 이라 .gitignore 에 있어 고쳐도 저장소에 안 남는다. 전투 중에는 원본(Opaque)을 쓰고,
#    죽는 순간 코드가 이 복사본으로 바꿔 끼운다 (AStormCharacterBase::DeathMaterials)
# 3. 복사본은 Material Attributes 출력 끝에 Break 와 Make Material Attributes 를 끼워 넣고,
#    Emissive 와 Opacity Mask 만 MF_DeathDissolve 를 거친다. Masked, 양면
# 다시 돌리면 함수와 복사본을 지우고 새로 만든다. 끝나면 저장한다
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

SRC_DIR = "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Materials"
DST_DIR = "/Game/Storm/Mob/Boss1/Materials"
MF_DIR = "/Game/Storm/FX"
MF_NAME = "MF_DeathDissolve"
SOURCES = ["M_Sevarog_Torso", "M_Sevarog_ArmourRuins", "M_Sevarog_Back_01", "M_Sevarog_Chainmail", "M_Sevarog_Hammer_01"]

# Break 에서 Make 로 그대로 넘기는 핀. Emissive 와 Opacity Mask 는 디졸브를 거치므로 뺀다.
# Cloth 셰이딩의 Fuzz Color 는 SubsurfaceColor, Cloth 값은 ClearCoat 핀으로 오간다.
# 재질 입력에 따로 연결하지 않고 Make 를 거치는 이유: Python 의 MaterialProperty 에 CustomData0, 1 과 Pixel Depth Offset 이 없다
PASS_THROUGH = [
    "BaseColor", "Metallic", "Specular", "Roughness", "Anisotropy", "Opacity", "Normal", "Tangent",
    "WorldPositionOffset", "SubsurfaceColor", "ClearCoat", "ClearCoatRoughness", "AmbientOcclusion",
    "Refraction", "PixelDepthOffset", "ShadingModel", "Displacement",
] + ["CustomizedUVs_%d" % i for i in range(8)]
REQUIRED = {"BaseColor", "Metallic", "Specular", "Roughness", "Normal", "SubsurfaceColor", "ClearCoat", "AmbientOcclusion"}


def connect(a, a_out, b, b_in):
    if not MEL.connect_material_expressions(a, a_out, b, b_in):
        raise RuntimeError("연결 실패: %s.%s 에서 %s.%s" % (a.get_class().get_name(), a_out, b.get_class().get_name(), b_in))


def delete_if_exists(path):
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)


def build_function():
    path = MF_DIR + "/" + MF_NAME
    delete_if_exists(path)
    mf = unreal.AssetToolsHelpers.get_asset_tools().create_asset(MF_NAME, MF_DIR, unreal.MaterialFunction, unreal.MaterialFunctionFactoryNew())
    mf.set_editor_property("description", "DeathFade 1에서 0으로 몸이 노이즈 모양으로 사라지고 경계가 빛난다")

    def node(cls, x, y):
        return MEL.create_material_expression_in_function(mf, cls, x, y)

    def scalar(name, value, x, y):
        e = node(unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", value)
        e.set_editor_property("group", "Death Dissolve")
        return e

    emissive_in = node(unreal.MaterialExpressionFunctionInput, -1400, -300)
    emissive_in.set_editor_property("input_name", "Emissive")
    emissive_in.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_VECTOR3)
    emissive_in.set_editor_property("use_preview_value_as_default", True)
    emissive_in.set_editor_property("sort_priority", 0)

    fade = scalar("DeathFade", 1.0, -1400, 0)
    dissolve_scale = scalar("DissolveScale", 0.04, -1400, 200)
    edge_width = scalar("EdgeWidth", 0.08, -1400, 400)
    edge_intensity = scalar("EdgeIntensity", 30.0, -1400, 600)
    edge_color = node(unreal.MaterialExpressionVectorParameter, -1400, 800)
    edge_color.set_editor_property("parameter_name", "EdgeColor")
    edge_color.set_editor_property("default_value", unreal.LinearColor(0.55, 0.75, 1.0, 1.0))
    edge_color.set_editor_property("group", "Death Dissolve")

    # 노이즈 N (0에서 1)
    position = node(unreal.MaterialExpressionPreSkinnedPosition, -1350, 150)
    # PreSkinnedPosition 은 버텍스 셰이더에서만 읽을 수 있어 보간기로 픽셀 셰이더에 넘긴다
    interp = node(unreal.MaterialExpressionVertexInterpolator, -1150, 150)
    connect(position, "", interp, "")
    scaled = node(unreal.MaterialExpressionMultiply, -950, 150)
    connect(interp, "", scaled, "A")
    connect(dissolve_scale, "", scaled, "B")
    noise = node(unreal.MaterialExpressionNoise, -750, 150)
    noise.set_editor_property("scale", 1.0)
    noise.set_editor_property("turbulence", False)
    noise.set_editor_property("output_min", 0.0)
    noise.set_editor_property("output_max", 1.0)
    connect(scaled, "", noise, "World Position")

    # 문턱 T = 1 - DeathFade. N이 T보다 큰 곳만 남는다
    threshold = node(unreal.MaterialExpressionOneMinus, -950, 0)
    connect(fade, "", threshold, "")
    d = node(unreal.MaterialExpressionSubtract, -550, 50)
    connect(noise, "", d, "A")
    connect(threshold, "", d, "B")

    # Opacity Mask = (N - T) + 0.5, 클립 값 0.5
    mask = node(unreal.MaterialExpressionAdd, -300, 50)
    connect(d, "", mask, "A")
    mask.set_editor_property("const_b", 0.5)

    # 경계 빛: N - T 가 EdgeWidth 안이면 빛난다. 처음(T가 0일 때)에는 노이즈가 낮은 곳이 빛나지 않게 T로 켠다
    ratio = node(unreal.MaterialExpressionDivide, -300, 250)
    connect(d, "", ratio, "A")
    connect(edge_width, "", ratio, "B")
    inv = node(unreal.MaterialExpressionOneMinus, -100, 250)
    connect(ratio, "", inv, "")
    band = node(unreal.MaterialExpressionSaturate, 100, 250)
    connect(inv, "", band, "")
    ramp_mul = node(unreal.MaterialExpressionMultiply, -300, 450)
    connect(threshold, "", ramp_mul, "A")
    ramp_mul.set_editor_property("const_b", 20.0)
    ramp = node(unreal.MaterialExpressionSaturate, -100, 450)
    connect(ramp_mul, "", ramp, "")
    glow = node(unreal.MaterialExpressionMultiply, 300, 350)
    connect(band, "", glow, "A")
    connect(ramp, "", glow, "B")

    color_rgb = node(unreal.MaterialExpressionComponentMask, -1150, 800)
    color_rgb.set_editor_property("r", True)
    color_rgb.set_editor_property("g", True)
    color_rgb.set_editor_property("b", True)
    color_rgb.set_editor_property("a", False)
    connect(edge_color, "", color_rgb, "")
    color = node(unreal.MaterialExpressionMultiply, -950, 700)
    connect(color_rgb, "", color, "A")
    connect(edge_intensity, "", color, "B")
    glow_color = node(unreal.MaterialExpressionMultiply, 500, 350)
    connect(glow, "", glow_color, "A")
    connect(color, "", glow_color, "B")

    emissive_sum = node(unreal.MaterialExpressionAdd, 700, -100)
    connect(emissive_in, "", emissive_sum, "A")
    connect(glow_color, "", emissive_sum, "B")

    out_emissive = node(unreal.MaterialExpressionFunctionOutput, 950, -100)
    out_emissive.set_editor_property("output_name", "Emissive")
    out_emissive.set_editor_property("sort_priority", 0)
    connect(emissive_sum, "", out_emissive, "")
    out_mask = node(unreal.MaterialExpressionFunctionOutput, 950, 50)
    out_mask.set_editor_property("output_name", "OpacityMask")
    out_mask.set_editor_property("sort_priority", 1)
    connect(mask, "", out_mask, "")

    MEL.update_material_function(mf)
    EAL.save_loaded_asset(mf)
    return mf


def build_death_material(source_name, mf):
    dst = DST_DIR + "/" + source_name + "_Death"
    delete_if_exists(dst)
    mat = EAL.duplicate_asset(SRC_DIR + "/" + source_name, dst)
    if not mat:
        raise RuntimeError("복사 실패: " + source_name)

    attrs = MEL.get_material_property_input_node(mat, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    attrs_out = MEL.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    if not attrs:
        raise RuntimeError(source_name + ": Material Attributes 입력이 비어 있다")

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("opacity_mask_clip_value", 0.5)
    # 구멍으로 안쪽 면이 보이게 한다
    mat.set_editor_property("two_sided", True)

    x = attrs.get_editor_property("material_expression_editor_x") + 400
    y = attrs.get_editor_property("material_expression_editor_y")
    brk = MEL.create_material_expression(mat, unreal.MaterialExpressionBreakMaterialAttributes, x, y)
    make = MEL.create_material_expression(mat, unreal.MaterialExpressionMakeMaterialAttributes, x + 700, y)
    connect(attrs, attrs_out, brk, "")
    for pin in PASS_THROUGH:
        if not MEL.connect_material_expressions(brk, pin, make, pin):
            if pin in REQUIRED:
                raise RuntimeError("%s: %s 연결 실패" % (source_name, pin))
            unreal.log_warning("%s: %s 핀 없음, 건너뜀" % (source_name, pin))

    call = MEL.create_material_expression(mat, unreal.MaterialExpressionMaterialFunctionCall, x + 350, y + 700)
    call.set_editor_property("material_function", mf)
    connect(brk, "EmissiveColor", call, "Emissive")
    connect(call, "Emissive", make, "EmissiveColor")
    connect(call, "OpacityMask", make, "OpacityMask")
    if not MEL.connect_material_property(make, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES):
        raise RuntimeError(source_name + ": Material Attributes 연결 실패")

    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    unreal.log("death material: %s" % dst)


mf = build_function()
for name in SOURCES:
    build_death_material(name, mf)
unreal.log("boss_death_materials: 함수 1개, 재질 %d개" % len(SOURCES))
