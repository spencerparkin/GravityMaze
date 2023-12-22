package com.spencer.gravitymaze

import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity

class MainMenu : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main_menu)

        findViewById<Button>(R.id.resume_game).setOnClickListener {
            Log.d("GRAVITY MAZE", "User tapped resume game button.")
            val intent = Intent(this, MainActivity::class.java)
            startActivity(intent)
        }
    }
}