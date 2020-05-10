extends "res://addons/godot-xr-tools/objects/Object_pickable.gd"

var material : SpatialMaterial

onready var original_transform = global_transform
var max_distance = 5.0

func _ready():
	material = $Cup.material
	material.emission_enabled = false
	
	# we can programatically set our margin smaller which is important with small objects like a cup
	$CollisionShape.shape.margin = 0.0001

func _update_highlight():
	if material:
		material.emission_enabled = closest_count > 0

func _physics_process(delta):
	if !is_picked_up():
		var delta_pos = global_transform.origin - original_transform.origin
		if delta_pos.length() > max_distance:
			# We've rolled away
			
			# Arresto Momentum!
			linear_velocity = Vector3.ZERO
			angular_velocity = Vector3.ZERO
			
			# Let's put it back but a little higher
			global_transform = original_transform
			global_transform.origin.y += 0.5 
			pass
			
