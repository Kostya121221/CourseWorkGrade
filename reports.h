#pragma once
#include "grade_store.h"
#include <string>
#include <vector>

namespace reports {
struct RiskAssessment {
    std::string level;
    std::vector<std::string> reasons;
};
RiskAssessment assessRisk(const Student& student);
void printRiskAssessment(const Student& student);
void print(const std::vector<Student>& rows);
std::vector<Student> groupReport(const std::vector<Student>& rows);
std::vector<Student> atRisk(const std::vector<Student>& rows);
void sortByGrade(std::vector<Student>& rows);
void printForecast(const std::vector<Student>& rows, const std::string& group);
void saveText(const std::vector<Student>& rows, const std::string& file);
}
