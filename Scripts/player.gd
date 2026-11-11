extends CharacterBody3D

const SPEED = 5.0
const JUMP_VELOCITY = 4.5

@export_range(0.1, 3.0, 0.1, "or_greater") var camera_horizontal_sens: float = 1
@export_range(0.1, 3.0, 0.1, "or_greater") var camera_vertical_sens: float = 1

@onready var camera: Camera3D = $Camera3D
var mouse_captured: bool = false
var move_dir: Vector2 # Input direction for movement
var look_dir: Vector2 # Input direction for look/aim

var holding_distance: float
var held_object = null
var mouse_motion : bool = false
var rotating : bool = false
var rotate_vector : Vector2

@export var weight_kg : float = 60

@onready var raycast: RayCast3D = $Camera3D/RayCast3D
@onready var held_object_dir: RayCast3D = $Camera3D/HeldObjectDir

var raycast_colliding : bool = false

@onready var crosshair : Label = $"../HUD/CenterContainer/Label"
@export var menu : PackedScene

@onready var animation_player : AnimationPlayer = $AnimationPlayer
var crouched : bool = false

func _ready() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	mouse_captured = true

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		mouse_motion = true
		look_dir = event.relative * 0.001
		if mouse_captured:
			if !rotating:
				camera.rotation.y -= look_dir.x * camera_horizontal_sens
				camera.rotation.x = clampf( camera.rotation.x - look_dir.y * camera_vertical_sens, deg_to_rad(-89), deg_to_rad(89) )
			else:
				rotate_vector += look_dir
	
	if Input.is_action_just_pressed("Exit"):
		if mouse_captured:
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
			mouse_captured = false
			get_parent().add_child( menu.instantiate() )
		else:
			Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
			mouse_captured = true
			$"../Menu".queue_free()
	
	if Input.is_action_just_pressed("Crouch"):
		if !crouched:
			crouched = true
			animation_player.play("Crouch")
		else:
			crouched = false
			animation_player.play_backwards("Crouch")
	
	#region GRAB
	if Input.is_action_just_pressed("Grab"):
		if raycast_colliding:
			if raycast.get_collider().is_class( "RigidBody3D" ):
				held_object = raycast.get_collider()
				held_object.start_holding()
				held_object_dir.global_basis = held_object.global_basis
				held_object.ray_dir = held_object.global_basis.z
				held_object.ray_side = held_object.global_basis.x
				holding_distance = clampf( ( raycast.global_position - held_object.global_position ).length(), 1.2, 2.5 )
				change_crosshair_color()
	
	if held_object != null:
		if Input.is_action_just_pressed("MoveAway"):
			holding_distance += 0.1
			holding_distance = clampf( holding_distance, 1.1, 2.5 )
		if Input.is_action_just_pressed("MoveCloser"):
			holding_distance -= 0.1
			holding_distance = clampf( holding_distance, 1.1, 2.5 )
		if Input.is_action_just_pressed("Launch"):
			if !rotating:
				held_object.launch( - raycast.global_basis.z )
				held_object = null
		if Input.is_action_just_pressed("Rotate"):
			rotating = true
			held_object_dir.reparent( held_object )
	else:
		if Input.is_action_just_pressed("Launch"):
			if !mouse_captured:
				Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
				mouse_captured = true
	
	if Input.is_action_just_released("Rotate"):
		if held_object != null:
			end_rotation()
	
	if Input.is_action_just_released("Grab"):
		if held_object != null:
			end_rotation()
			held_object.stop_holding()
			held_object = null
			change_crosshair_color()
	#endregion

func _physics_process(delta: float) -> void:
	# Add the gravity.
	if not is_on_floor():
		velocity += get_gravity() * delta

	# Handle jump.
	if Input.is_action_just_pressed("Jump") and is_on_floor():
		velocity.y = JUMP_VELOCITY

	# Get the input direction and handle the movement/deceleration.
	# As good practice, you should replace UI actions with custom gameplay actions.
	var input_dir := Input.get_vector("Left", "Right", "Forward", "Backwards")
	var direction := (camera.global_transform.basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()
	if direction:
		velocity.x = direction.x * SPEED
		velocity.z = direction.z * SPEED
	else:
		velocity.x = move_toward(velocity.x, 0, SPEED)
		velocity.z = move_toward(velocity.z, 0, SPEED)
	
	if held_object == null:
		if raycast_colliding != raycast.is_colliding():
			raycast_colliding = raycast.is_colliding()
			change_crosshair_color()
	
	#Handle movement of Grabbed Object
	if held_object != null:
		held_object.grab_point = raycast.global_position + ( - raycast.global_basis.z ) * holding_distance
		held_object.ray_dir = - held_object_dir.global_basis.z
		held_object.ray_side = - held_object_dir.global_basis.x
		if rotating:
			if mouse_motion:
				held_object.rotate_vector = rotate_vector
			else:
				held_object.rotate_vector = Vector2.ZERO
			rotate_vector = Vector2.ZERO
			mouse_motion = false
	
	_push_away_rigid_bodies()
	move_and_slide()

func _push_away_rigid_bodies() -> void:
	for i in get_slide_collision_count():
		var c:= get_slide_collision( i )
		var collider = c.get_collider()
		if collider is RigidBody3D:
			var push_dir := -c.get_normal()
			var velocity_diff_in_push_dir = self.velocity.dot( push_dir ) - collider.linear_velocity.dot( push_dir )
			velocity_diff_in_push_dir = max( 0., velocity_diff_in_push_dir )
			var mass_ratio = min( 1., weight_kg / collider.mass )
			if mass_ratio < 0.25:
				continue
			push_dir.y = 0
			var push_force = mass_ratio * 1.0 #magic number
			collider.apply_impulse( push_dir * velocity_diff_in_push_dir * push_force, c.get_position() - collider.global_position )

func change_crosshair_color() -> void:
	if held_object != null:
		crosshair.label_settings.font_color = Color(0.0, 0.081, 0.69, 1.0)
		return
	if raycast_colliding:
		crosshair.label_settings.font_color = Color(0.0, 0.672, 0.691, 1.0)
	else:
		crosshair.label_settings.font_color = Color(1.0, 1.0, 1.0, 1.0)

func end_rotation() -> void:
	rotating = false
	held_object_dir.reparent( camera )
	rotate_vector = Vector2.ZERO
	held_object.rotate_vector = rotate_vector
