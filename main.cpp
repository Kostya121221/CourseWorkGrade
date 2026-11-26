#include "grade_store.h"
#include "reports.h"
#include "auth.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <string>

namespace {
int readInt(const std::string& prompt, int low, int high) {
    for (;;) {
        std::cout << prompt;
        int value{};
        if (std::cin >> value && value >= low && value <= high) return value;
        std::cout << "Введите целое число от " << low << " до " << high << ".\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin >> std::ws, value);
    return value;
}
}

int main() {
    try {
        if (!authenticate()) return 1;
        GradeStore store("students.dat");
        for (;;) {
            std::cout << "\n=== Анализ успеваемости студентов ИБ ===\n"
                      << "1. Показать записи  2. Добавить  3. Изменить  4. Удалить\n"
                      << "5. Найти по группе  6. Сформировать отчет  7. Сортировать/фильтр\n"
                      << "8. Анализ и прогноз  9. Экспорт отчета  10. Оценить риск по ID  0. Выход\n";
            const int choice = readInt("Выбор: ", 0, 10);
            if (choice == 0) break;
            if (choice == 1) reports::print(store.all());
            else if (choice == 2) {
                Student s;
                s.id = store.nextId();
                s.name = readLine("ФИО: "); s.group = readLine("Группа: ");
                s.subject = readLine("Дисциплина: ");
                s.term = readInt("Семестр (1-12): ", 1, 12);
                s.grade = readInt("Оценка (2-5): ", 2, 5);
                s.attendance = readInt("Посещаемость (0-100%): ", 0, 100);
                s.labs = readInt("Выполнено лабораторных (0-30): ", 0, 30);
                store.add(s);
            } else if (choice == 3) {
                const int id = readInt("ID записи: ", 1, 1000000000);
                Student s;
                s.id = id; s.name = readLine("ФИО: "); s.group = readLine("Группа: ");
                s.subject = readLine("Дисциплина: "); s.term = readInt("Семестр (1-12): ", 1, 12);
                s.grade = readInt("Оценка (2-5): ", 2, 5);
                s.attendance = readInt("Посещаемость (0-100%): ", 0, 100);
                s.labs = readInt("Выполнено лабораторных (0-30): ", 0, 30);
                if (!store.update(id, s)) std::cout << "Запись не найдена.\n";
            } else if (choice == 4) {
                if (!store.remove(readInt("ID записи: ", 1, 1000000000))) std::cout << "Запись не найдена.\n";
            } else if (choice == 5) reports::print(store.byGroup(readLine("Группа: ")));
            else if (choice == 6) reports::print(reports::groupReport(store.all()));
            else if (choice == 7) {
                auto rows = store.all();
                const int mode = readInt("1 - сортировка по оценке, 2 - фильтр риска: ", 1, 2);
                if (mode == 1) reports::sortByGrade(rows);
                else rows = reports::atRisk(rows);
                reports::print(rows);
            } else if (choice == 8) reports::printForecast(store.all(), readLine("Группа для анализа: "));
            else if (choice == 9) {
                const auto report = reports::groupReport(store.all());
                reports::saveText(report, "report.txt");
                std::cout << "Сводный отчет сохранен в report.txt\n";
            } else if (choice == 10) {
                const int id = readInt("ID студента: ", 1, 1000000000);
                const auto rows = store.all();
                const auto found = std::find_if(rows.begin(), rows.end(), [id](const Student& s) { return s.id == id; });
                if (found == rows.end()) std::cout << "Запись не найдена.\n";
                else reports::printRiskAssessment(*found);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
