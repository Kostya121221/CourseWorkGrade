#include "auth.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <functional>

namespace {
std::uint64_t hashPassword(const std::string& text, std::uint64_t salt) {
    // FNV-1a с солью для учебного примера; не предназначен для защиты реальных учетных записей.
    std::uint64_t h=14695981039346656037ULL ^ salt;
    for(unsigned char c:text){h^=c;h*=1099511628211ULL;}
    return h;
}
bool configured() { std::ifstream in("auth.dat",std::ios::binary); return in.good() && in.seekg(0,std::ios::end).tellg()==static_cast<std::streamoff>(2*sizeof(std::uint64_t)); }
}

bool authenticate() {
    if(!configured()) {
        std::cout<<"Первый запуск: задайте пароль пользователя (не используйте реальный пароль).\nПароль: ";
        std::string password; std::cin>>password;
        if(password.size()<4) throw std::runtime_error("Пароль должен содержать не менее 4 символов");
        const auto salt=static_cast<std::uint64_t>(std::hash<std::string>{}(password+"grade-forecast-local-salt"));
        const auto digest=hashPassword(password,salt);
        std::ofstream out("auth.dat",std::ios::binary|std::ios::trunc);
        if(!out) throw std::runtime_error("Не удалось сохранить хеш пароля");
        out.write(reinterpret_cast<const char*>(&salt),sizeof salt); out.write(reinterpret_cast<const char*>(&digest),sizeof digest);
        if(!out) throw std::runtime_error("Ошибка записи файла учетных данных");
        std::cout<<"Хеш пароля сохранен.\n"; return true;
    }
    std::cout<<"Пароль: "; std::string password; std::cin>>password;
    std::ifstream in("auth.dat",std::ios::binary); std::uint64_t salt{},expected{};
    if(!in.read(reinterpret_cast<char*>(&salt),sizeof salt)||!in.read(reinterpret_cast<char*>(&expected),sizeof expected)) throw std::runtime_error("Поврежден файл учетных данных");
    if(hashPassword(password,salt)!=expected){std::cout<<"Неверный пароль.\n";return false;}
    return true;
}
