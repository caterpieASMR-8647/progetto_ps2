extends RigidBody3D

var audio_clip = preload("res://impact_cut.wav") 

var audio_node = null
var collision_last_tick : bool = false

var picked : bool = false
var grab_point : Vector3
var ray_dir : Vector3
var ray_side : Vector3
var rotate_vector : Vector2
@export var follow_grab_strength : float = 30.0
@export var launch_force : float = 10.0

var aabb : AABB
var volume : float

@onready var base_gravity_scale : float = self.gravity_scale

func _ready() -> void:
	audio_node = $AudioStreamPlayer3D
	audio_node.stop()
	
	follow_grab_strength = follow_grab_strength * mass / 2
	launch_force = launch_force * mass / 2
	
	for child in find_children( "*" ):
		if child.is_class("MeshInstance3D"):
			aabb = child.get_aabb()
	
	volume = aabb.get_volume() * 5
	if volume > 0.01 and volume <= 0.1:
		volume = 0.5
	elif volume <= 0.01:
		volume = 0.1

func _physics_process(delta: float) -> void:
	if picked:
		if global_position.distance_to( grab_point ) < 5.0:
			linear_velocity = Vector3.ZERO
			apply_central_impulse( ( grab_point - global_position ) * follow_grab_strength )
		elif global_position.distance_to( grab_point ) >= 5.0:
			self.gravity_scale = base_gravity_scale
			picked = false
		angular_velocity = Vector3.ZERO
		if rotate_vector == Vector2.ZERO:
			apply_torque_impulse( ray_dir.cross( global_basis.z ) * volume )
			apply_torque_impulse( ray_side.cross( global_basis.x ) * volume )
		else:
			apply_torque_impulse( Vector3( rotate_vector.y, rotate_vector.x, 0 ) * volume * 5 )
		
	var collision = move_and_collide( self.linear_velocity * delta, true );
	if collision && !collision_last_tick:
		audio_node.volume_db = clampf( (linear_velocity.length() * linear_velocity.length()) - 60, -80.0, 0.0 )
		play_sound( audio_clip )
	collision_last_tick = collision != null

func start_holding(  ):
	picked = true
	self.gravity_scale = 0

func stop_holding():
	self.gravity_scale = base_gravity_scale
	picked = false

func launch( launch_dir : Vector3 ):
	stop_holding()
	apply_central_impulse( launch_dir * launch_force )

func play_sound( sound, _sound_position = null ):
	if audio_node.playing:
		return
	
	if sound == null:
		print("Audio not set")
		queue_free()
	
	audio_node.stream = sound
	
	audio_node.play()
