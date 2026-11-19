using UnrealBuildTool;

// 에디터 전용 모듈. VW.uplugin에서 Type이 Editor라서 게임 패키징에는 들어가지 않는다.
public class VWEditor : ModuleRules
{
	public VWEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"VW",
			"Slate",
			"SlateCore",
			"InputCore",
			"EditorFramework",
			"EditorStyle",
			"UnrealEd",
			"LevelEditor",
			"InteractiveToolsFramework",
			"EditorInteractiveToolsFramework",
		});
	}
}
