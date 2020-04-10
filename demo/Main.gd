extends Spatial

func _process(delta):
	# Test for escape to close application, space to reset our reference frame
	if (Input.is_key_pressed(KEY_ESCAPE)):
		get_tree().quit()
	elif (Input.is_key_pressed(KEY_SPACE)):
		# Calling center_on_hmd will cause the ARVRServer to adjust all tracking data so the player is centered on the origin point looking forward
		ARVRServer.center_on_hmd(true, true)

func _on_Right_Hand_button_pressed(button):
	if button == JOY_OCULUS_BY:
		$FirstPerson/Guardian.visible = !$FirstPerson/Guardian.visible
