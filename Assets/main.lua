
trinket.log "Hello..."

-- create some assets

local mat_checkboard = trinket.import_material "checkerboard.mat"
local mat_surface = trinket.import_material "surface.mat"
local mesh_plane = trinket.create_plane_mesh("plane", 4)
local mesh_box = trinket.create_cube_mesh("cube", 0.25)

local mesh_mecha = trinket.import_mesh "mecha.mesh"
local mat_mecha = trinket.import_material "mecha.mat"

-- place floor in scene

local floor = trinket.create_object("floor")
trinket.attach_rendermesh_to(floor, mesh_plane, mat_checkboard, false)
trinket.set_rotation(floor, -90, 0, 0)
trinket.add_ground_plane()

-- place mecha in scene

local mecha = trinket.create_object("mecha")
trinket.attach_rendermesh_to(mecha, mesh_mecha, mat_mecha)

-- scatter boxes

function rand_range(u, v) 
	return u + (v - u) * math.random() 
end

local narnia = trinket.create_sublevel("Narnia")
local names = { "lucy", "peter", "edmund", "susan" }
for idx,name in ipairs(names) do 
	local obj = trinket.create_object(name, narnia)
	local x = rand_range(-2.0, 2.0)
	local y = rand_range(2, 4)
	local z = rand_range(-2.0, 2.0)
	trinket.set_position(obj, x, y, z)

	local rx = rand_range(-180, 180)
	local ry = rand_range(-180, 180)
	local rz = rand_range(-180, 180)
	trinket.set_rotation(obj, rx, ry, rz)

	trinket.attach_rendermesh_to(obj, mesh_box, mat_surface)
	trinket.attach_rigidbody_to(obj)
	trinket.attach_boxcollider_to(obj, 0.25, 1.0)
end

trinket.set_light_direction(0.5, -1, 0.5)
trinket.set_pov_position(0, 4, -8)
-- trinket.set_time_dilation(0.5)

local yaw = 0
local pitch = 30

function clamp(x, min, max) 
	if x < min then 
		return min 
	elseif x > max then 
		return max 
	else 
		return x
	end
end

local mecha_rotation = 0

function tick(dt)

	-- camera rotation
	local rx, ry = trinket.get_right_stick()
	yaw = yaw + rx * 120 * dt
	pitch = clamp(pitch - ry * 120 * dt, -90, 90)
	trinket.set_pov_rotation(pitch, yaw, 0)

	-- camera translation
	local lx, ly = trinket.get_left_stick()
	local lt = trinket.get_left_trigger()
	local rt = trinket.get_right_trigger()
	trinket.translate_pov_local(
		5 * lx * dt, 
		5 * (rt - lt) * dt, 
		5 * ly * dt
	)

	-- wireframe test
	-- local yaw = 90 * trinket.get_raw_time()
	-- wireframe.set_color(1, 1, 0)
	-- wireframe.set_position(0,0,0)
	-- wireframe.set_rotation(0, yaw, 0)
	-- wireframe.line_to(0,1,0)
	-- wireframe.draw_cube(0.025)
	-- wireframe.line_to(1,1,0)
	-- wireframe.draw_cube(0.025)
	-- wireframe.line_to(1,0,0)
	-- wireframe.line_to(0.5,0,0)
	-- wireframe.line_to(0.5,0.5, 0)
	-- wireframe.draw_cube(0.025)

	mecha_rotation = mecha_rotation + 10 * dt
	trinket.set_rotation(mecha,0, mecha_rotation, 0)

end

trinket.log "...World"
