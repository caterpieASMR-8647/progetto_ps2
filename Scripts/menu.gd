extends Control

@onready var player : CharacterBody3D = $"../Player"

func _on_resume_button_button_down() -> void:
	player.mouse_captured = true
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	queue_free()

func _on_exit_button_button_down() -> void:
	get_tree().quit()
