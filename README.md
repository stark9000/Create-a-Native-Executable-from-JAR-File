🚀 Jar2Exe: Convert Java JAR to Native Executable

A tool that embeds a Java JAR file into a native C launcher, creating a single executable that runs your Java application without requiring users to run java -jar.

✨ Features
Single executable output - No separate JAR files to distribute

XOR encryption - Basic protection for embedded JAR

Cross-platform - Generates C code for Windows and Linux

No installation required - Users don't need Java installed (JVM must be present)

Automatic JVM discovery - Finds Java installation via JAVA_HOME

📋 Prerequisites
For Building (Developer Machine)
Java 8+ JDK

C compiler (gcc, MinGW, or MSVC)

For Running (End User Machine)
Java Runtime Environment (JRE) 8+ installed

JAVA_HOME environment variable set

🛠️ How It Works
text
┌─────────────┐     ┌──────────────┐     ┌─────────────┐     ┌──────────────┐
│  Your JAR   │────▶│  Encrypt     │────▶│  Generate   │────▶│  Compile     │
│  File       │     │  + Embed     │     │  C Launcher │     │  to EXE      │
└─────────────┘     └──────────────┘     └─────────────┘     └──────────────┘
Your JAR is encrypted with XOR

Encrypted data is embedded into a C template

C code decrypts and runs the JAR via JNI

Final executable contains everything

📁 Project Structure
text
jar2exe/
├── Jar2ExePacker.java      # Java packer tool
├── launcher_template.c     # C launcher template
├── build.bat               # Windows build script
├── build.sh                # Linux build script
└── output/                 # Generated files
    ├── output.c            # Generated C code
    └── launcher.exe        # Final executable


📝 Code Walkthrough
Java Packer (Jar2ExePacker.java)
java
// 1. Read JAR file
byte[] jarBytes = readAllBytes(new FileInputStream(inputJar));

// 2. Build payload (class mappings + JAR data)
ByteArrayOutputStream buf = new ByteArrayOutputStream();
DataOutputStream dos = new DataOutputStream(buf);
dos.writeInt(0);  // 0 classes for no obfuscation
dos.writeInt(jarBytes.length);
dos.write(jarBytes);

// 3. Encrypt with XOR
byte[] key = new byte[32];
new SecureRandom().nextBytes(key);
byte[] encrypted = xorEncrypt(payload, key);

// 4. Replace placeholders in C template
String result = template
    .replace("%%KEY%%", toHexLiterals(key))
    .replace("%%DATA%%", toHexLiterals(encrypted))
    .replace("%%MAIN_CLASS%%", mainClassJvm);
C Launcher (launcher_template.c)
The C template handles:

Decryption - XOR decrypts the embedded payload

Temp file - Writes JAR to temporary location

JVM loading - Dynamically loads jvm.dll (Windows) or libjvm.so (Linux)

Execution - Calls your main class via JNI

c
// Key decryption
static u8* xor_decrypt(const u8 *key, size_t key_len, 
                       const u8 *data, size_t data_len, size_t *out_len) {
    u8 *plain = malloc(data_len);
    for (size_t i = 0; i < data_len; i++) {
        plain[i] = data[i] ^ key[i % key_len];
    }
    return plain;
}

// Parse and execute
int main(int argc, char **argv) {
    // Decrypt → Parse → Write temp JAR → Load JVM → Run main()
}
🔧 Advanced Usage
Adding Obfuscation
To add class name obfuscation, modify Jar2ExePacker.java:

java
// Instead of writing 0 classes, write actual mappings
Map<String, String> mappings = getObfuscationMappings();
dos.writeInt(mappings.size());
for (Map.Entry<String, String> entry : mappings.entrySet()) {
    dos.writeShort(entry.getKey().length());
    dos.writeBytes(entry.getKey());
    dos.writeShort(entry.getValue().length());
    dos.writeBytes(entry.getValue());
}
Changing JVM Options
Edit the options array in launcher_template.c:

c
JavaVMOption options[] = {
    { (char *)classpath, NULL },
    { (char *)"-Xmx512m", NULL },  // Increase heap size
    { (char *)"-Dfile.encoding=UTF-8", NULL }  // Add properties
};
vm_args.nOptions = 3;
🐛 Troubleshooting
"Failed to load JVM library"
Solution: Set JAVA_HOME correctly

bash
# Windows
set JAVA_HOME=C:\Program Files\Java\jdk1.8.0_162

# Linux
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk
"Main class not found"
Solution: Verify main class path uses slashes, not dots

java
// Correct
String mainClass = "com/mycompany/MyApp";  // Uses /

// Incorrect  
String mainClass = "com.mycompany.MyApp";  // Uses .
Compilation errors
Windows MinGW: Ensure JVM headers are accessible

bash
gcc -O2 -o launcher.exe output.c ^
    -I"%JAVA_HOME%\include" ^
    -I"%JAVA_HOME%\include\win32" ^
    -L"%JAVA_HOME%\jre\bin\server" ^
    -ljvm
📊 Performance
Operation	Time
XOR Decryption	~50ms for 10MB
JVM Startup	~200-500ms
Total Overhead	~250-550ms
⚠️ Limitations
JVM must be installed on target system

No true "statically linked" Java

Basic XOR encryption (not for security-critical apps)

Windows/Linux only (add MacOS with libjvm.dylib)

🔒 Security Note
The XOR encryption in this tool is obfuscation only - not real encryption. The key is embedded in the executable and can be extracted. For real protection, consider:

Commercial Java obfuscators (ProGuard, Allatori)

Native compilation with GraalVM

Custom packers with anti-debugging

📄 License
MIT License - Use freely for personal and commercial projects.

 