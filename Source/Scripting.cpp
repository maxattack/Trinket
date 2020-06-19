// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Scripting.h"
#include "ObjectHandle.h"
#include "World.h"
#include <lua.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/quaternion.hpp>

//------------------------------------------------------------------------------------------
// scripting helper methods/macros

inline static ScriptVM* GetVM(lua_State* lua) { 
	return *static_cast<ScriptVM**>(lua_getextraspace(lua)); 
}

#define SCRIPT_PREAMBLE              \
	let vm = GetVM(lua);             \
	World& world = *vm->GetWorld();  

#define SCENE_OBJ_METHOD_PREAMBLE                          \
	let vm = GetVM(lua);                                   \
	World& world = *vm->GetWorld();                        \
	let obj = check_obj(lua, ObjectTag::SCENE_OBJECT, 1);  \
	let hierarchy = world.scene.GetSublevelHierarchyFor(obj.id);

static ObjectHandle check_obj(lua_State* lua, ObjectTag tag, int narg) {
	ObjectHandle obj (lua_touserdata(lua, narg));
	if (obj.tag != tag) {
		luaL_argerror(lua, narg, "Wrong Object Type");
		return ObjectHandle(ForceInit::Default);
	}
	return obj;
}

static bool lua_check_boolean(lua_State* lua, int narg) {
	luaL_argcheck(lua, narg <= lua_gettop(lua), narg, "Expected Boolean");
	return !!lua_toboolean(lua, narg);
}

static bool lua_check_boolean_opt(lua_State* lua, int narg, bool opt_def = false) {
	if (narg > lua_gettop(lua))
		return opt_def;
	return !!lua_toboolean(lua, narg);
}

static void lua_pushobj(lua_State* lua, ObjectTag type, ObjectID id) {
	lua_pushlightuserdata(lua, ObjectHandle(type, id).p);
}

static float lua_checkfloat(lua_State* lua, int narg) {
	return (float) luaL_checknumber(lua, narg);
}

static vec2 lua_check_vec2(lua_State* lua, int narg) { 
	return vec2(
		lua_checkfloat(lua, narg), 
		lua_checkfloat(lua, narg + 1)
	); 
}

static vec3 lua_check_vec3(lua_State* lua, int narg) { 
	return vec3(
		lua_checkfloat(lua, narg), 
		lua_checkfloat(lua, narg + 1), 
		lua_checkfloat(lua, narg + 2)
	); 
}

static quat lua_check_euler(lua_State* lua, int narg) {
	return quat(glm::radians(lua_check_vec3(lua, narg)));
}

static vec4 lua_check_color_opt_alpha(lua_State* lua, int narg) { 
	vec4 result;
	result.r = lua_checkfloat(lua, narg);
	result.g = lua_checkfloat(lua, narg+1);
	result.b = lua_checkfloat(lua, narg+2);
	result.a = narg+3 > lua_gettop(lua) ? 1.f : lua_checkfloat(lua, narg+3);
	return result;
}

static void lua_pushvec2(lua_State* lua, vec2 v) {
	lua_pushnumber(lua, v.x);
	lua_pushnumber(lua, v.y);
}

static void lua_pushvec3(lua_State* lua, vec3 v) {
	lua_pushnumber(lua, v.x);
	lua_pushnumber(lua, v.y);
	lua_pushnumber(lua, v.z);
}

//------------------------------------------------------------------------------------------
// script entry-points

extern "C" {

static int l_log(lua_State* lua) {
	let str = luaL_checkstring(lua, 1);
	std::cout << "[SCRIPT] " << str << std::endl;
	return 0;
}

static int l_is_valid(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	lua_pushboolean(lua, obj.id.IsFingerprinted() ? world.db.IsValid(obj.id) : world.scene.IsValid(obj.id));
	return 1;
}

static int l_object_count(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushinteger(lua, world.scene.GetSceneObjectCount());
	return 1;
}

static int l_create_object(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let name = luaL_checkstring(lua, 1);
	let result = world.scene.CreateObject(name);
	
	ObjectHandle parent;
	if (lua_gettop(lua) > 1) {
		parent.p = lua_touserdata(lua, 2);
		let bTypeOK = parent.IsSceneObject() || parent.IsSublevel();
		if (!bTypeOK)
			return luaL_argerror(lua, 2, "Expected Scene Object or Sublevel");
	}

	if (parent.IsSceneObject())
		world.scene.GetSublevelHierarchyFor(parent.id)->TryAdd(result, parent.id);
	else if (parent.IsSublevel())
		world.scene.GetHierarchy(parent.id)->TryAdd(result);
	else
		world.scene.GetHierarchyByIndex(0)->TryAdd(result);
	
	lua_pushobj(lua, ObjectTag::SCENE_OBJECT, result);
	return 1;
}

static int l_create_sublevel(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let name = luaL_checkstring(lua, 1);
	lua_pushobj(lua, ObjectTag::SUBLEVEL_OBJECT, world.scene.CreateSublevel(name));
	return 1;

}

static int l_default_sublevel(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushobj(lua, ObjectTag::SUBLEVEL_OBJECT,  world.scene.GetSublevelByIndex(0));
	return 1;
}

static int l_find_object(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let name = luaL_checkstring(lua, 1);
	let result = world.scene.FindObject(name);
	if (world.scene.IsSceneObject(result))
		lua_pushobj(lua, ObjectTag::SCENE_OBJECT, result);
	else if (world.scene.IsSublevel(result))
		lua_pushobj(lua, ObjectTag::SUBLEVEL_OBJECT, result);
	else
		lua_pushlightuserdata(lua, nullptr);
	return 1;
}

static int l_object_at(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let idx = static_cast<int32>(luaL_checkinteger(lua, 1));
	let n = world.scene.GetSceneObjectCount();
	if (idx < 1 || idx > n)
		return luaL_argerror(lua, 1, "Index out of Range");
	lua_pushobj(lua, ObjectTag::SCENE_OBJECT, world.scene.GetSceneObjectByIndex(idx - 1));
	return 1;
}

static int l_position(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	let pos = hierarchy->GetScenePose(obj.id)->position;
	lua_pushvec3(lua, pos);
	return 3;
}

static int l_relative_position(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	let pos = hierarchy->GetRelativePose(obj.id)->position;
	lua_pushvec3(lua, pos);
	return 3;
}

static int l_set_pose_mask(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	const PoseMask mask(
		lua_check_boolean(lua, 2),
		lua_check_boolean(lua, 3),
		lua_check_boolean(lua, 4)
	);
	hierarchy->SetMask(obj.id, mask);
	return 0;
}

static int l_set_position(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	hierarchy->SetScenePosition(obj.id, lua_check_vec3(lua, 2));
	return 0;
}

static int l_set_rotation(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	hierarchy->SetSceneRotation(obj.id, lua_check_euler(lua, 2));
	return 0;
}

static int l_create_material(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let name = luaL_checkstring(lua, 1);

	eastl::string nameStr(name);
	let vertexFile = nameStr + ".vsh";
	let pixelFile = nameStr + ".psh";

	Material* result = nullptr;

	if (lua_gettop(lua) > 1) {
		let textureName = luaL_checkstring(lua, 2);
		let textureFile = "Assets/" + eastl::string(textureName) + ".psd";

		TextureArg targ;
		targ.variableName = "g_Texture";
		targ.pTexture = world.gfx.CreateTexture(textureName);
		targ.pTexture->AllocFile(textureFile.c_str());
		targ.pTexture->TryLoad(&world.gfx);
		targ.pTexture->Dealloc();

		MaterialArgs args;
		args.vertexShaderFile = vertexFile.c_str();
		args.pixelShaderFile = pixelFile.c_str();
		args.numTextures = 1;
		args.pTextureArgs = &targ;
		result = world.gfx.CreateMaterial(name, args);

	} else {
		MaterialArgs args;
		args.vertexShaderFile = vertexFile.c_str();
		args.pixelShaderFile = pixelFile.c_str();
		args.numTextures = 0;
		args.pTextureArgs = nullptr;
		result = world.gfx.CreateMaterial(name, args);
	}


	if (result)
		lua_pushobj(lua, ObjectTag::MATERIAL_ASSET, result->ID());
	else 
		lua_pushobj(lua, ObjectTag::UNDEFINED, OBJECT_NIL);
	return 1;
}

static int l_create_cube_mesh(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let name = luaL_checkstring(lua, 1);
	let extent = (float) luaL_checknumber(lua, 2);
	let mesh = world.gfx.CreateMesh(name);
	mesh->GetSubmesh(0).AllocCube(extent);
	for (auto& it : mesh->GetSubmesh(0)) {
		let hue = float(rand() % 360);
		it.color = vec4(glm::rgbColor(vec3(hue, 1.f, 1.f)), 1.f);
	}
	mesh->GetSubmesh(0).TryLoad(&world.gfx);
	mesh->GetSubmesh(0).Dealloc();
	lua_pushobj(lua, ObjectTag::MESH_ASSET, mesh->ID());
	return 1;
}

static int l_create_plane_mesh(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let name = luaL_checkstring(lua, 1);
	let extent = (float) luaL_checknumber(lua, 2);
	let plane = world.gfx.CreateMesh(name);
	plane->GetSubmesh(0).AllocPlaneXY(extent);
	plane->GetSubmesh(0).TryLoad(&world.gfx);
	plane->GetSubmesh(0).Dealloc();
	lua_pushobj(lua, ObjectTag::MESH_ASSET, plane->ID());
	return 1;
}

static int l_attach_rendermesh_to(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	let mesh = check_obj(lua, ObjectTag::MESH_ASSET, 2);
	let material = check_obj(lua, ObjectTag::MATERIAL_ASSET, 3);
	let shadow = lua_check_boolean_opt(lua, 4, true);
	let pMesh = world.gfx.GetMesh(mesh.id);
	let pMaterial = world.gfx.GetMaterial(material.id);
	RenderMeshData rmd { pMesh, pMaterial, shadow };	
	let result = world.gfx.TryAttachRenderMeshTo(obj.id, rmd);
	lua_pushboolean(lua, result);
	return 1;
}

static int l_add_ground_plane(lua_State* lua) {
	SCRIPT_PREAMBLE;
	world.phys.TryAddGroundPlane();
	return 0;
}

static int l_attach_rigidbody_to(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	let result = world.phys.TryAttachRigidbodyTo(obj.id);
	lua_pushboolean(lua, result);
	return 1;
}

static int l_attach_boxcollider_to(lua_State* lua) {
	SCENE_OBJ_METHOD_PREAMBLE;
	let extent = lua_checkfloat(lua, 2);
	let density = lua_checkfloat(lua, 3);
	let result = world.phys.TryAttachBoxTo(obj.id, extent, density);
	lua_pushboolean(lua, result);
	return 1;
}

static int l_get_pov_position(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let result = world.gfx.GetPOV().pose.position;
	lua_pushvec3(lua, result);
	return 3;
}

static int l_set_pov_position(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let pos =lua_check_vec3(lua, 1);
	world.gfx.SetEyePosition(pos);
	return 0;
}

int l_translate_pov_local(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let offset = lua_check_vec3(lua, 1);
	let& pose = world.gfx.GetPOV().pose;
	world.gfx.SetEyePosition(pose.position + pose.rotation * offset);
	return 0;
}

static int l_set_pov_rotation(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let rot = lua_check_euler(lua, 1);
	world.gfx.SetEyeRotation(rot);
	return 0;
}

static int l_set_light_direction(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let dir = glm::normalize(lua_check_vec3(lua, 1));
	world.gfx.SetLightDirection(dir);
	return 0;
}

static int l_get_left_stick(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushvec2(lua, world.input.GetStickL());
	return 2;
}

static int l_get_right_stick(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushvec2(lua, world.input.GetStickR());
	return 2;
}

static int l_get_left_trigger(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushnumber(lua, world.input.GetTriggerValueL());
	return 1;
}

static int l_get_right_trigger(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushnumber(lua, world.input.GetTriggerValueR());
	return 1;
}

static int l_get_game_time(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushnumber(lua, world.input.GetTime());
	return 1;
}

static int l_get_raw_time(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushnumber(lua, float(world.input.GetTicks()));
	return 1;
}

static int l_set_time_dilation(lua_State* lua) {
	SCRIPT_PREAMBLE;
	world.input.SetTimeDilation(lua_checkfloat(lua, 1));
	return 0;
}


static const struct luaL_Reg lib_trinket[] = {
	{ "log",                  l_log                  },
	{ "is_valid",             l_is_valid             },
	
	// object functions
	{ "object_count",         l_object_count         },
	{ "create_object",        l_create_object        },
	{ "create_sublevel",      l_create_sublevel      },
	{ "default_sublevel",     l_default_sublevel     },
	{ "find_object",          l_find_object          },
	{ "object_at",            l_object_at            },

	// input 
	{ "get_left_stick",       l_get_left_stick       },
	{ "get_right_stick",      l_get_right_stick      },
	{ "get_left_trigger",     l_get_left_trigger     },
	{ "get_right_trigger",    l_get_right_trigger    },
	{ "get_game_time",        l_get_game_time        },
	{ "get_raw_time",         l_get_raw_time         },
	{ "set_time_dilation",    l_set_time_dilation    },

	// hierarchy functions
	{ "position",             l_position             },
	{ "relative_position",    l_relative_position    },
	{ "set_pose_mask",        l_set_pose_mask        },
	{ "set_position",         l_set_position         },
	{ "set_rotation",         l_set_rotation         },

	// gfx functions
	{ "get_pov_position",     l_get_pov_position     },
	{ "set_pov_position",     l_set_pov_position     },
	{ "set_pov_rotation",     l_set_pov_rotation     },
	{ "translate_pov_local",  l_translate_pov_local  },
	{ "set_light_direction",  l_set_light_direction  },

	// material functions
	{ "create_material",      l_create_material      },

	// mesh functions
	{ "create_cube_mesh",     l_create_cube_mesh     },
	{ "create_plane_mesh",    l_create_plane_mesh    },
	{ "attach_rendermesh_to", l_attach_rendermesh_to },

	// physics functions
	{ "add_ground_plane",      l_add_ground_plane      },
	{ "attach_rigidbody_to",   l_attach_rigidbody_to   },
	{ "attach_boxcollider_to", l_attach_boxcollider_to },

	{ nullptr, nullptr }
};

#if TRINKET_TEST

static int l_wireframe_set_position(lua_State* lua) {
	SCRIPT_PREAMBLE;
	vm->wireframePosition = lua_check_vec3(lua, 1);
	return 0;
}

static int l_wireframe_set_rotation(lua_State* lua) {
	SCRIPT_PREAMBLE;
	vm->wireframeRotation = lua_check_euler(lua, 1);
	return 0;
}

static int l_wireframe_set_color(lua_State* lua) {
	SCRIPT_PREAMBLE;
	vm->wireframeColor = lua_check_color_opt_alpha(lua, 1);
	return 0;
}

static int l_wireframe_line_to(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let nextPos = lua_check_vec3(lua, 1);
	world.gfx.DrawDebugLine(vm->wireframeColor, vm->wireframePosition, nextPos);
	vm->wireframePosition = nextPos;
	return 0;
}

static int l_wireframe_get_position(lua_State* lua) {
	SCRIPT_PREAMBLE;
	lua_pushvec3(lua, vm->wireframePosition);
	return 3;
}

static int l_wireframe_draw_cube(lua_State* lua) {
	SCRIPT_PREAMBLE;
	let sz = lua_checkfloat(lua, 1);
	let pos = vm->wireframePosition;
	let rot = vm->wireframeRotation;
	let col = vm->wireframeColor;

	// draw bottom
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, -sz, -sz), pos + rot * vec3(sz, -sz, -sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, -sz, -sz), pos + rot * vec3(-sz, sz, -sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(sz, -sz, -sz), pos + rot * vec3(sz, sz, -sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, sz, -sz), pos + rot * vec3(sz, sz, -sz));

	// draw top
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, -sz, sz), pos + rot * vec3(sz, -sz, sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, -sz, sz), pos + rot * vec3(-sz, sz, sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(sz, -sz, sz), pos + rot * vec3(sz, sz, sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, sz, sz), pos + rot * vec3(sz, sz, sz));

	// draw connections
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, -sz, -sz), pos + rot * vec3(-sz, -sz, sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(sz, -sz, -sz), pos + rot * vec3(sz, -sz, sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(-sz, sz, -sz), pos + rot * vec3(-sz, sz, sz));
	world.gfx.DrawDebugLine(col, pos + rot * vec3(sz, sz, -sz), pos + rot * vec3(sz, sz, sz));

	return 0;
}

#else
static int l_wireframe_set_position(lua_State* lua) { return 0; }
static int l_wireframe_set_rotation(lua_State* lua) { return 0; }
static int l_wireframe_set_color(lua_State* lua) { return 0; }
static int l_wireframe_line_to(lua_State* lua) { return 0; }
static int l_wireframe_get_position(lua_State* lua) { lua_pushvec3(lua, vec3(0,0,0)); return 3; }
static ine l_wireframe_draw_cube(lua_State* lua) { return 0; }
#endif

static const struct luaL_Reg lib_wireframe[] = {
	{ "set_position", l_wireframe_set_position },
	{ "set_rotation", l_wireframe_set_rotation },
	{ "set_color",    l_wireframe_set_color    },
	{ "line_to",      l_wireframe_line_to      },
	{ "get_position", l_wireframe_get_position },
	{ "draw_cube",    l_wireframe_draw_cube    },
	{ nullptr, nullptr }
};

}

#undef SCRIPT_PREAMBLE
#undef SCENE_OBJ_METHOD_PREAMBLE

//------------------------------------------------------------------------------------------
// Script VM External API

ScriptVM::ScriptVM(World *aWorld) : pWorld(aWorld)
{
	lua = luaL_newstate();
	CHECK_ASSERT(lua != nullptr);
	*static_cast<ScriptVM**>(lua_getextraspace(lua)) = this;
	luaL_openlibs(lua);
	luaL_register(lua, "trinket", lib_trinket);
	luaL_register(lua, "wireframe", lib_wireframe);
}

ScriptVM::~ScriptVM() {
	if (lua)
		lua_close(lua);
}

void ScriptVM::RunScript(const char* path) {
	using namespace std;
	if (luaL_dofile(lua, path) != LUA_OK) {
		size_t len;
		let msg = lua_tolstring(lua, -1, &len);
		cout << "[SCRIPT] Error: " << msg << endl;
	}
}

void ScriptVM::Update() {
	using namespace std;

	if (pWorld->input.GetDeltaTicks() == 0)
		return;

	let type = lua_getglobal(lua, "tick");
	if (type == LUA_TFUNCTION) {
		lua_pushnumber(lua, pWorld->input.GetDeltaTime());
		if (lua_pcall(lua, 1, 0, 0)) {
			cout << "[SCRIPT] Error: " << lua_tostring(lua, -1) << endl;
		}
	} else {
		lua_pop(lua, 1);
	}

}
