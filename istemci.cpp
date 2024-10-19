#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int soket = 0;
struct sockaddr_in sunucu_adres;
std::string ip = "127.0.0.1"; // Varsayılan IP adresi
int port = 8080;

void hedefe_baglan() {
    // Soket oluştur
    soket = socket(AF_INET, SOCK_STREAM, 0);
    if (soket < 0) {
        std::cerr << "Soket oluşturulamadı!" << std::endl;
        return;
    }

    sunucu_adres.sin_family = AF_INET;
    sunucu_adres.sin_port = htons(port);

    // IP adresini kullanıcıdan al
    std::cout << "Bağlanılacak IP adresini girin (varsayılan: 127.0.0.1): ";
    std::cin >> ip;

    // IP adresini ayarla
    if (inet_pton(AF_INET, ip.c_str(), &sunucu_adres.sin_addr) <= 0) {
        std::cerr << "Geçersiz adres!" << std::endl;
        return;
    }

    // Sunucuya bağlan
    if (connect(soket, (struct sockaddr*)&sunucu_adres, sizeof(sunucu_adres)) < 0) {
        std::cerr << "Bağlantı başarısız!" << std::endl;
        return;
    }

    std::cout << "Sunucuya başarıyla bağlanıldı." << std::endl;
}

int main() {
    hedefe_baglan();

    // Bağlantıyı kapat
    close(soket);
    return 0;
}
