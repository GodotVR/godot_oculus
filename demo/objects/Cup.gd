extends "res://addons/godot-xr-tools/objects/Object_pickable.gd"

var material : SpatialMaterial

func _ready():
	material = $Cup.material
	material.emission_enabled = false

func _update_highlight():
	if material:
		material.emission_enabled = closest_count > 0
