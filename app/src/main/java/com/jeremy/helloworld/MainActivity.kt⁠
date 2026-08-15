package com.jeremy.helloworld

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.BufferedReader
import java.io.InputStreamReader

class MainActivity : AppCompatActivity() {
    private lateinit var textView: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        textView = TextView(this).apply {
            text = "Hello World!\nChecking APatch Root (Background)..."
            textSize = 18f
            setPadding(32, 32, 32, 32)
        }
        
        setContentView(textView)

        // Run root checks on a background thread to prevent UI freezing/ANR crashes
        lifecycleScope.launch {
            val rootResult = checkRootStatusAsync()
            textView.text = "Hello World!\n\n$rootResult"
        }
    }

    private suspend fun checkRootStatusAsync(): String = withContext(Dispatchers.IO) {
        try {
            // Try standard su path first
            val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
            val reader = BufferedReader(InputStreamReader(process.inputStream))
            val output = StringBuilder()
            var line: String?
            while (reader.readLine().also { line = it } != null) {
                output.append(line).append("\n")
            }
            process.waitFor()
            
            if (output.isNotEmpty()) {
                "Root Access Granted:\n$output"
            } else {
                "Root execution returned empty output."
            }
        } catch (e: Exception) {
            "Root Access Denied or Unavailable:\n${e.message}"
        }
    }
}
