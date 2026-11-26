#include "reports.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <stdexcept>

namespace reports {
RiskAssessment assessRisk(const Student& student) {
    RiskAssessment assessment;
    int points = 0;
    if (student.grade <= 2) { points += 3; assessment.reasons.push_back("неудовлетворительная оценка"); }
    else if (student.grade == 3) { points += 2; assessment.reasons.push_back("оценка 3"); }
    if (student.attendance < 60) { points += 2; assessment.reasons.push_back("посещаемость ниже 60%"); }
    else if (student.attendance < 75) { points += 1; assessment.reasons.push_back("посещаемость ниже 75%"); }
    if (student.labs < 3) { points += 2; assessment.reasons.push_back("выполнено менее 3 лабораторных"); }
    else if (student.labs < 5) { points += 1; assessment.reasons.push_back("выполнено менее 5 лабораторных"); }

    assessment.level = points >= 5 ? "высокий" : points >= 2 ? "средний" : "низкий";
    if (assessment.reasons.empty()) assessment.reasons.push_back("критерии риска не выявлены");
    return assessment;
}

void printRiskAssessment(const Student& student) {
    const auto assessment = assessRisk(student);
    std::cout << "\nОценка риска для " << student.name << " (" << student.group << ")\n"
              << "Уровень: " << assessment.level << "\nПричины:\n";
    for (const auto& reason : assessment.reasons) std::cout << " - " << reason << '\n';
    std::cout << "Это индикатор для внимания преподавателя, а не автоматическое решение об успеваемости.\n";
}

void print(const std::vector<Student>& rows) {
    if (rows.empty()) { std::cout << "Записей нет.\n"; return; }
    std::cout << std::left << std::setw(5) << "ID" << std::setw(25) << "ФИО" << std::setw(12) << "Группа"
              << std::setw(24) << "Дисциплина" << std::setw(8) << "Семестр" << std::setw(8) << "Оценка"
              << std::setw(12) << "Посещ.,%" << "Лаб.\n";
    for (const auto& s:rows) std::cout << std::left << std::setw(5) << s.id << std::setw(25) << s.name.substr(0,24)
        << std::setw(12) << s.group.substr(0,11) << std::setw(24) << s.subject.substr(0,23) << std::setw(8) << s.term
        << std::setw(8) << s.grade << std::setw(12) << s.attendance << s.labs << '\n';
}
std::vector<Student> groupReport(const std::vector<Student>& rows) {
    std::map<std::string,std::vector<Student>> groups;
    for (const auto& s:rows) groups[s.group].push_back(s);
    std::vector<Student> result;
    for (const auto& [group,items]:groups) {
        const double avg=std::accumulate(items.begin(),items.end(),0.0,[](double a,const Student& s){return a+s.grade;})/items.size();
        const int attendance=static_cast<int>(std::lround(std::accumulate(items.begin(),items.end(),0.0,[](double a,const Student& s){return a+s.attendance;})/items.size()));
        const int risk=static_cast<int>(std::count_if(items.begin(),items.end(),[](const Student& s){return s.grade<=3 || s.attendance<60;}));
        Student summary; summary.id=static_cast<int>(items.size()); summary.name="Средний балл "+std::to_string(avg).substr(0,4);
        summary.group=group; summary.subject="Риск: "+std::to_string(risk)+"/"+std::to_string(items.size());
        summary.term=0; summary.grade=static_cast<int>(std::lround(avg)); summary.attendance=attendance; summary.labs=0;
        result.push_back(summary);
    }
    return result;
}
std::vector<Student> atRisk(const std::vector<Student>& rows) {
    std::vector<Student> result; for(const auto& s:rows) if(s.grade<=3 || s.attendance<60 || s.labs<3) result.push_back(s); return result;
}
void sortByGrade(std::vector<Student>& rows) {
    std::stable_sort(rows.begin(),rows.end(),[](const Student& a,const Student& b){return a.grade>b.grade;});
}
void printForecast(const std::vector<Student>& rows,const std::string& group) {
    std::map<int,std::vector<int>> terms;
    for(const auto& s:rows) if(s.group==group) terms[s.term].push_back(s.grade);
    if(terms.empty()) {std::cout<<"Для группы не найдены оценки.\n";return;}
    std::vector<double> means;
    for(const auto& [term,grades]:terms) means.push_back(std::accumulate(grades.begin(),grades.end(),0.0)/grades.size());
    const double current=means.back();
    double predicted=current;
    if(means.size()>1) {
        const double n=static_cast<double>(means.size()); double sx=0,sy=0,sxx=0,sxy=0;
        for(std::size_t i=0;i<means.size();++i){const double x=i+1,y=means[i];sx+=x;sy+=y;sxx+=x*x;sxy+=x*y;}
        const double slope=(n*sxy-sx*sy)/(n*sxx-sx*sx);
        predicted=std::clamp(current+slope,2.0,5.0);
    }
    const double attendance=std::accumulate(rows.begin(),rows.end(),0.0,[&](double sum,const Student& s){return sum+(s.group==group?s.attendance:0);});
    const auto count=std::count_if(rows.begin(),rows.end(),[&](const Student& s){return s.group==group;});
    const double avgAttendance=count?attendance/count:0;
    std::cout<<"Анализ группы "<<group<<": "<<means.size()<<" семестров с оценками.\n"
      <<"Средний балл последнего наблюдаемого семестра: "<<std::fixed<<std::setprecision(2)<<current<<".\n"
      <<"Линейная экстраполяция среднего на следующий семестр: "<<predicted<<" (шкала 2-5).\n"
      <<"Средняя посещаемость по имеющимся записям: "<<avgAttendance<<"%.\n"
      <<"Модель описательная: прогноз рассчитан по средним оценкам семестров, без обучающей выборки; при одном семестре используется последнее среднее.\n";
}
void saveText(const std::vector<Student>& rows,const std::string& file) {
    std::ofstream out(file); if(!out) throw std::runtime_error("Не удалось создать текстовый отчет");
    out<<"ID\tФИО\tГруппа\tДисциплина\tСеместр\tОценка\tПосещаемость\tЛабораторные\n";
    for(const auto& s:rows) out<<s.id<<'\t'<<s.name<<'\t'<<s.group<<'\t'<<s.subject<<'\t'<<s.term<<'\t'<<s.grade<<'\t'<<s.attendance<<'\t'<<s.labs<<'\n';
    if(!out) throw std::runtime_error("Ошибка записи текстового отчета");
}
}
