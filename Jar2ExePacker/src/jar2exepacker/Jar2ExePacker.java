package jar2exepacker;

import java.io.*;
import java.security.SecureRandom;

public class Jar2ExePacker {

    public static void main(String[] args) throws Exception {
        File inputJar = new File("./sampleApp.jar");
        String mainClass = "sampleapp.SampleApp";
        File templateFile = new File("./launcher_template.c");
        File outputFile = new File("./output.c");

        System.out.println("[*] Processing JAR: " + inputJar.getAbsolutePath());

        if (!inputJar.exists()) {
            System.err.println("Error: Input JAR not found!");
            System.exit(1);
        }

        // Read original JAR as-is (no obfuscation)
        byte[] jarBytes = readAllBytes(new FileInputStream(inputJar));
        System.out.println("[*] Original JAR size: " + jarBytes.length);

        // Create simple payload: just the JAR bytes
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(buf);

        // Write 0 classes (no obfuscation mapping)
        dos.writeInt(0);
        // Write JAR size and data
        dos.writeInt(jarBytes.length);
        dos.write(jarBytes);
        dos.flush();

        byte[] payload = buf.toByteArray();
        System.out.println("[*] Payload size: " + payload.length);

        // Generate XOR key
        byte[] key = new byte[32];
        new SecureRandom().nextBytes(key);

        // XOR encrypt
        byte[] encrypted = new byte[payload.length];
        for (int i = 0; i < payload.length; i++) {
            encrypted[i] = (byte) (payload[i] ^ key[i % key.length]);
        }

        System.out.println("[*] Encrypted payload size: " + encrypted.length);

        String template = readFile(templateFile);
        String mainClassJvm = mainClass.replace('.', '/');

        String result = template
                .replace("%%KEY%%", toHexLiterals(key))
                .replace("%%KEY_LEN%%", String.valueOf(key.length))
                .replace("%%DATA%%", toHexLiterals(encrypted))
                .replace("%%DATA_LEN%%", String.valueOf(encrypted.length))
                .replace("%%MAIN_CLASS%%", mainClassJvm);

        writeFile(outputFile, result);
        System.out.println("[+] Launcher written: " + outputFile.getAbsolutePath());
    }

    private static String toHexLiterals(byte[] data) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < data.length; i++) {
            sb.append(String.format("0x%02x", data[i] & 0xFF));
            if (i < data.length - 1) {
                sb.append(",");
                if ((i + 1) % 12 == 0) {
                    sb.append("\n    ");
                }
            }
        }
        return sb.toString();
    }

    private static byte[] readAllBytes(InputStream is) throws IOException {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        byte[] data = new byte[4096];
        int nRead;
        while ((nRead = is.read(data, 0, data.length)) != -1) {
            buffer.write(data, 0, nRead);
        }
        return buffer.toByteArray();
    }

    private static String readFile(File file) throws IOException {
        StringBuilder sb;
        try (BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(file), "UTF-8"))) {
            sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) {
                sb.append(line);
                sb.append("\n");
            }
        }
        return sb.toString();
    }

    private static void writeFile(File file, String data) throws IOException {
        try (BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file), "UTF-8"))) {
            bw.write(data);
        }
    }
}
