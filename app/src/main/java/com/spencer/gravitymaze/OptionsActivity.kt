package com.spencer.gravitymaze

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.SeekBar
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.*
import java.io.File

// A view-model approach is better, but I can barely write kotlin code, so let's just do this for now.
class OptionsActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_options)

        findViewById<Button>(R.id.buttonSaveAndExit).setOnClickListener {
            Log.d("GRAVITY MAZE", "User save and exit button.")
            pullOptionsFromControls()
            val intent = Intent(this, MainMenu::class.java)
            startActivity(intent)
        }

        pushOptionsToControls()
    }

    private val gravityMax : Float = 300.0f
    private val bounceMax : Float = 1.0f
    private fun pushOptionsToControls() {
        val file = File("$filesDir/options.json")
        val jsonString = if(file.exists()) {
            file.readText()
        } else {
            application.assets.open("default_options.json").bufferedReader().use {
                it.readText()
            }
        }

        val options = Json.parseToJsonElement(jsonString).jsonObject

        val gravity = options["gravity"]!!.jsonPrimitive.float
        val bounce = options["bounce"]!!.jsonPrimitive.float

        val gravityBar = findViewById<SeekBar>(R.id.seekBarGravity)
        val bounceBar = findViewById<SeekBar>(R.id.seekBarBounce)

        gravityBar.progress = ((gravity / gravityMax) * gravityBar.max.toFloat()).toInt()
        bounceBar.progress = ((bounce / bounceMax) * bounceBar.max.toFloat()).toInt()
    }

    private fun pullOptionsFromControls() {

        val gravityBar = findViewById<SeekBar>(R.id.seekBarGravity)
        val bounceBar = findViewById<SeekBar>(R.id.seekBarBounce)

        val gravity = (gravityBar.progress.toFloat() / gravityBar.max.toFloat()) * gravityMax
        val bounce = (bounceBar.progress.toFloat() / bounceBar.max.toFloat()) * bounceMax

        val jsonString : String = "{" +
                "\"gravity\": $gravity," +
                "\"bounce\": $bounce" +
                "}"

        val file = File("$filesDir/options.json")
        file.writeText(jsonString)
    }
}