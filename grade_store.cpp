#include "grade_store.h"
#include <cstdint>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace {
constexpr std::uint32_t magic = 0x47464231; // GFB1
constexpr std::uint32_t version = 1;
void writeString(std::ostream& out, const std::string& s) {
    if (s.size() > 4096) throw std::runtime_error("Строковое поле длиннее 4096 байт");
    const auto n = static_cast<std::uint32_t>(s.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof n);
    out.write(s.data(), n);
}
std::string readString(std::istream& in) {
    std::uint32_t n{};
    if (!in.read(reinterpret_cast<char*>(&n), sizeof n) || n > 4096) throw std::runtime_error("Поврежден бинарный файл: длина поля");
    std::string s(n, '\0');
    if (n && !in.read(s.data(), n)) throw std::runtime_error("Поврежден бинарный файл: данные строки");
    return s;
}
template<class T> void put(std::ostream& out, const T& x) { out.write(reinterpret_cast<const char*>(&x), sizeof x); }
template<class T> void get(std::istream& in, T& x) { if (!in.read(reinterpret_cast<char*>(&x), sizeof x)) throw std::runtime_error("Поврежден бинарный файл"); }
}

GradeStore::GradeStore(std::string file) : file_(std::move(file)) { load(); }
void GradeStore::load() {
    std::ifstream in(file_, std::ios::binary);
    if (!in) return;
    std::uint32_t m{}, v{}, count{};
    get(in,m); get(in,v); get(in,count);
    if (m != magic || v != version || count > 1000000) throw std::runtime_error("Неизвестный формат или неверное число записей");
    rows_.reserve(count);
    for (std::uint32_t i=0; i<count; ++i) {
        Student s;
        get(in,s.id); s.name=readString(in); s.group=readString(in); s.subject=readString(in);
        get(in,s.term); get(in,s.grade); get(in,s.attendance); get(in,s.labs);
        if (s.id < 1 || s.term < 1 || s.term > 12 || s.grade < 2 || s.grade > 5 || s.attendance < 0 || s.attendance > 100 || s.labs < 0 || s.labs > 30)
            throw std::runtime_error("В записи обнаружены значения вне допустимого диапазона");
        for (const auto& old : rows_) if (old.id == s.id) throw std::runtime_error("В файле повторяется идентификатор записи");
        rows_.push_back(std::move(s));
    }
    if (in.peek() != std::char_traits<char>::eof()) throw std::runtime_error("В бинарном файле есть лишние данные");
}
void GradeStore::save() const {
    const std::string tmp = file_ + ".tmp";
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Не удалось создать файл базы данных");
    put(out,magic); put(out,version); const auto count=static_cast<std::uint32_t>(rows_.size()); put(out,count);
    for (const auto& s: rows_) {
        put(out,s.id); writeString(out,s.name); writeString(out,s.group); writeString(out,s.subject);
        put(out,s.term); put(out,s.grade); put(out,s.attendance); put(out,s.labs);
    }
    out.flush();
    if (!out) throw std::runtime_error("Ошибка записи бинарного файла");
    out.close();
    if (std::rename(tmp.c_str(),file_.c_str()) != 0) {
        std::remove(file_.c_str());
        if (std::rename(tmp.c_str(),file_.c_str()) != 0) throw std::runtime_error("Не удалось заменить файл базы данных (students.dat.tmp)");
    }
}
std::vector<Student> GradeStore::all() const { return rows_; }
std::vector<Student> GradeStore::byGroup(const std::string& group) const {
    std::vector<Student> result; for (const auto& s: rows_) if (s.group == group) result.push_back(s); return result;
}
int GradeStore::nextId() const { int n=1; for (const auto& s:rows_) if (s.id>=n) n=s.id+1; return n; }
void GradeStore::add(const Student& student) { rows_.push_back(student); try { save(); } catch (...) { rows_.pop_back(); throw; } }
bool GradeStore::update(int id,const Student& student) {
    for (auto& s:rows_) if (s.id==id) { const auto old=s; s=student; s.id=id; try {save();} catch (...) {s=old;throw;} return true; } return false;
}
bool GradeStore::remove(int id) {
    for (std::size_t i=0;i<rows_.size();++i) if (rows_[i].id==id) { const auto old=rows_[i]; rows_.erase(rows_.begin()+static_cast<std::ptrdiff_t>(i)); try {save();} catch (...) {rows_.insert(rows_.begin()+static_cast<std::ptrdiff_t>(i),old);throw;} return true; } return false;
}
