# Final_Project

## 🚀 Getting Started

To get this project up and running on your ESP32 board, follow the steps below:

### 1️⃣ Install ESP32 Board in Arduino IDE

1. Open the **Arduino IDE**.
2. Go to **Tools > Board > Board Manager**.
3. Search for **ESP32** and install **version 3.2.0**.

   ![Board Manager Screenshot](https://github.com/user-attachments/assets/7ef0fd62-e4e8-4794-a714-5694a27981ad)

### 2️⃣ Connect Your Board

- Use a **stable USB-C connection** to connect your ESP32 board to your computer.
- In the **Board selection menu**, choose:  
  **`ESP32-S3-DevKit`**

---

## ⚠️ Wi-Fi Library Notice

This project currently uses a **C library for Eduroam Wi-Fi connection**.  
When running the code, you might encounter a **red deprecation warning**. This is expected.

- ✅ The current (deprecated) library **still works**.
- ❌ The **suggested replacement library** in the warning message **does not work properly**, so we continue to use the current setup.

Feel free to contribute a fix or improvement for the Wi-Fi connection method if you discover a better alternative.
