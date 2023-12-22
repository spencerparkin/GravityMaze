package com.spencer.gravitymaze

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.widget.SeekBar
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.*

class OptionsActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_options)

        val jsonString = application.assets.open("options.json").bufferedReader().use {
            it.readText()
        }
        val options = Json.parseToJsonElement(jsonString).jsonObject

        val gravity = options["gravity"]!!.jsonPrimitive.float
        var bounce = options["bounce"]!!.jsonPrimitive.float

        // TODO: This is wrong.
        findViewById<SeekBar>(R.id.seekBarGravity).alpha = gravity
        findViewById<SeekBar>(R.id.seekBarBounce).alpha = bounce

        val i : Int = 0
    }
}