function create_weapons(scene)
	local weapons = {}
	
	weapons.m4a1 = {
		current_rounds = 300,
		is_automatic = true,
		bullets_once = 1,
		bullet_speed = minmax(2000, 2000),
		
		shooting_interval_ms = 100,
		spread_degrees = 0,
		shake_radius = 9.5,
		shake_spread_degrees = 45,
		
		bullet_distance_offset = vec2(50, 0),
		
		bullet_entity = {
			render = {
				model = scene.bullet_sprite,
				layer = render_layers.BULLETS
			},
			
			physics = {
				body_type = Box2D.b2_dynamicBody,
	
				body_info = {
					filter = filter_bullets,
					shape_type = physics_info.RECT,
					rect_size = scene.bullet_sprite.size,
					fixed_rotation = true,
					density = 0.1,
					bullet = true
				}
			}
		},				
		
		max_bullet_distance = 2000	
	}
	
	scene.weapons = weapons
end