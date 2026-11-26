#pragma once
#include <string>
#include <vector>

struct Student {
    int id{};
    std::string name;
    std::string group;
    std::string subject;
    int term{};
    int grade{};
    int attendance{};
    int labs{};
};

class GradeStore {
public:
    explicit GradeStore(std::string file);
    std::vector<Student> all() const;
    std::vector<Student> byGroup(const std::string& group) const;
    int nextId() const;
    void add(const Student& student);
    bool update(int id, const Student& student);
    bool remove(int id);
private:
    std::string file_;
    std::vector<Student> rows_;
    void load();
    void save() const;
};
