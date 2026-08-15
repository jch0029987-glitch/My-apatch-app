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
            text = "Requesting APatch SuperKey & Root Access...\nCheck your root manager prompt."
            textSize = 16f
            setPadding(32, 32, 32, 32)
        }
        
        setContentView(textView)

        // Run SuperKey verification asynchronously to avoid UI freezing
        lifecycleScope.launch {
            val result = verifySuperKeyAndRoot()
            textView.text = result
        }
    }

    private suspend fun verifySuperKeyAndRoot(): String = withContext(Dispatchers.IO) {
        try {
            // Executing the root shell prompt which forces APatch to validate 
            // the SuperKey/authorization state for this app package.
            val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
            
            val reader = BufferedReader(InputStreamReader(process.inputStream))
            val errorReader = BufferedReader(InputStreamReader(process.errorStream))
            
            val output = StringBuilder()
            var line: String?
            while (reader.readLine().also { line = it } != null) {
                output.append(line).append("\n")
            }
            
            val errorOutput = StringBuilder()
            while (errorReader.readLine().also { line = it } != null) {
                errorOutput.append(line).append("\n")
            }
            
            val exitCode = process.waitFor()

            // Detailed error checking and response handling
            return@withContext when {
                exitCode == 0 && output.contains("uid=0(root)") -> {
                    "SUCCESS: SuperKey Authenticated & Root Granted!\n\nUser Context:\n$output"
                }
                exitCode != 0 -> {
                    val reason = if (errorOutput.isNotEmpty()) errorOutput.toString() else "User denied root request or invalid SuperKey."
                    "ACCESS DENIED / ERROR:\n$reason\n(Exit Code: $exitCode)"
                }
                else -> {
                    "WARNING: Process exited cleanly, but root context was not fully confirmed.\nOutput: $output"
                }
            }
            
        } catch (e: java.io.IOException) {
            return@withContext "ERROR: Execution failed. The 'su' binary could not be found or launched.\nDetails: ${e.localizedMessage}"
        } catch (e: SecurityException) {
            return@withContext "SECURITY ERROR: Policy violation or missing permission context.\nDetails: ${e.localizedMessage}"
        } catch (e: Exception) {
            return@withContext "UNKNOWN EXCEPTION:\n${e.localizedMessage}"
        }
    }
}
