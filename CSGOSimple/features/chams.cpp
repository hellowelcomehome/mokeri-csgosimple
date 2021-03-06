﻿#include "chams.hpp"
#include <fstream>

#include "../valve_sdk/csgostructs.hpp"
#include "../options.hpp"
#include "../hooks.hpp"
#include "../helpers/input.hpp"

Chams::Chams() {
	materialRegular = g_MatSystem->FindMaterial("debug/debugambientcube");
	materialFlat = g_MatSystem->FindMaterial("debug/debugdrawflat");
	std::ofstream("csgo\\materials\\material_glow.vmt") << R"#("VertexLitGeneric" 
{
 "$additive"               "1"
 "$envmap"                 "models/effects/cube_white"
 "$envmaptint"             "[1 1 1]"
 "$envmapfresnel"          "1"
 "$envmapfresnelminmaxexp" "[0 1 2]"
 "$ignorez"                "0"
}
)#";
	materialGlow = g_MatSystem->FindMaterial("material_glow");
}

Chams::~Chams() {
}

void Chams::OverrideMaterialPlayer(bool ignoreZ, bool wireframe, int type, const Color& rgba) {
	IMaterial* material = nullptr;

	switch (g_Options.chams_material)
	{
	case 0:
		material = materialRegular;
		break;
	case 1:
		material = materialFlat;
		break;
	case 2:
		material = materialFlat;
		break;
	case 3:
		material = materialGlow;
		break;
	}

	if (g_Options.chams_material == 2) {
		material = materialFlat;
		material->AlphaModulate(0.45f);
	}
	else {
		material->AlphaModulate(
			rgba.a() / 255.0f);
	}

	bool bFound = false;
	IMaterialVar* pMatVar = material->FindVar("$envmaptint", &bFound);
	if (bFound)
		(*(void(__thiscall**)(int, float, float, float))(*(DWORD*)pMatVar + 0x2c))((uintptr_t)pMatVar, rgba.r() / 255.f, rgba.g() / 255.f, rgba.b() / 255.f);

	material->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, wireframe);
	material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, ignoreZ);
	material->ColorModulate(
		rgba.r() / 255.0f,
		rgba.g() / 255.0f,
		rgba.b() / 255.0f);
	material->IncrementReferenceCount();
	g_MdlRender->ForcedMaterialOverride(material);
}

void Chams::OverrideMaterialArms(bool ignoreZ, bool wireframe, int type, const Color& rgba) {
	IMaterial* amaterial = nullptr;

	switch (g_Options.chams_material_arms)
	{
	case 0:
		amaterial = materialRegular;
		break;
	case 1:
		amaterial = materialFlat;
		break;
	case 2:
		amaterial = materialFlat;
		break;
	case 3:
		amaterial = materialGlow;
		break;
	}

	if (g_Options.chams_material_arms == 2) {
		amaterial = materialFlat;
		amaterial->AlphaModulate(0.45f);
	}
	else {
		amaterial->AlphaModulate(
			rgba.a() / 255.0f);
	}

	bool bFound = false;
	IMaterialVar* pMatVar = amaterial->FindVar("$envmaptint", &bFound);
	if (bFound)
		(*(void(__thiscall**)(int, float, float, float))(*(DWORD*)pMatVar + 0x2c))((uintptr_t)pMatVar, rgba.r() / 255.f, rgba.g() / 255.f, rgba.b() / 255.f);

	amaterial->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, wireframe);
	amaterial->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, ignoreZ);
	amaterial->ColorModulate(
		rgba.r() / 255.0f,
		rgba.g() / 255.0f,
		rgba.b() / 255.0f);
	amaterial->IncrementReferenceCount();
	g_MdlRender->ForcedMaterialOverride(amaterial);
}

void Chams::OnDrawModelExecute(IMatRenderContext* ctx,const DrawModelState_t& state,const ModelRenderInfo_t& info,matrix3x4_t* matrix)
{
	static auto fnDME = Hooks::mdlrender_hook.get_original<decltype(&Hooks::hkDrawModelExecute)>(index::DrawModelExecute);

	const auto mdl = info.pModel;

	bool is_arm = strstr(mdl->szName, "arms") != nullptr;
	bool is_player = strstr(mdl->szName, "models/player") != nullptr;
	bool is_sleeve = strstr(mdl->szName, "sleeve") != nullptr;
	//bool is_weapon = strstr(mdl->szName, "weapons/v_")  != nullptr;

	if (is_player && g_Options.chams_player_enabled) {
		// 
		// Draw player Chams.
		// 
		auto ent = C_BasePlayer::GetPlayerByIndex(info.entity_index);

		if (ent && g_LocalPlayer && ent->IsAlive()) {
			const auto enemy = ent->m_iTeamNum() != g_LocalPlayer->m_iTeamNum();
			if (!enemy && g_Options.chams_player_enemies_only)
				return;

			const auto clr_front = enemy ? Color(g_Options.color_chams_player_enemy_visible) : Color(g_Options.color_chams_player_ally_visible);
			const auto clr_back = enemy ? Color(g_Options.color_chams_player_enemy_occluded) : Color(g_Options.color_chams_player_ally_occluded);

			if (g_Options.chams_player_ignorez) {
				OverrideMaterialPlayer(
					true,
					g_Options.chams_player_wireframe,
					g_Options.chams_material,
					clr_back);
				fnDME(g_MdlRender, 0, ctx, state, info, matrix);
				OverrideMaterialPlayer(
					false,
					g_Options.chams_player_wireframe,
					g_Options.chams_material,
					clr_front);
			}
			else {
				OverrideMaterialPlayer(
					false,
					g_Options.chams_player_wireframe,
					g_Options.chams_material,
					clr_front);
			}
		}
	}
	else if (is_sleeve && g_Options.chams_arms_enabled) {
		auto material = g_MatSystem->FindMaterial(mdl->szName, TEXTURE_GROUP_MODEL);
		if (!material)
			return;
		// 
		// Remove sleeves when drawing Chams.
		// 
		material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
		g_MdlRender->ForcedMaterialOverride(material);
	}
	else if (is_arm) {
		auto material = g_MatSystem->FindMaterial(mdl->szName, TEXTURE_GROUP_MODEL);
		if (!material)
			return;
		if (g_Options.misc_no_hands) {
			// 
			// No hands.
			// 
			material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
			g_MdlRender->ForcedMaterialOverride(material);
		}
		else if (g_Options.chams_arms_enabled) {
			if (g_Options.chams_arms_ignorez) {
				OverrideMaterialArms(
					true,
					g_Options.chams_arms_wireframe,
					g_Options.chams_material_arms,
					Color(g_Options.color_chams_arms_occluded));
				fnDME(g_MdlRender, 0, ctx, state, info, matrix);
				OverrideMaterialArms(
					false,
					g_Options.chams_arms_wireframe,
					g_Options.chams_material_arms,
					Color(g_Options.color_chams_arms_visible));
			}
			else {
				OverrideMaterialArms(
					false,
					g_Options.chams_arms_wireframe,
					g_Options.chams_material_arms,
					Color(g_Options.color_chams_arms_visible));
			}
		}
	}
}