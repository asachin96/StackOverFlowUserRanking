// File with utility functions
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <stdlib.h> /* atoi */
#include <ctime>
#include <assert.h>

static const int MONTHS_PER_YEAR = 12;

void printDate(struct std::tm &date) {
    std::cout << (date.tm_year + 1900) << '-'
                << (date.tm_mon + 1) << '-'
                <<  date.tm_mday
                << std::endl;
}

// Days per month split into normal year and leap year
const int daysPerMonth[2][13] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0}
};

// is Leap year
bool isLeapYear(int year) {
    return (year % 4 == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}

// On most machines, n/m and n%m return negative numbers when n is negative
// This is a problem in many cases, e.g., timestamps and intervals
// Compute n divided_by m with positive remainder, even if n is negative
int kDiv(int n, int m) {
    return (n < 0 ? ~(~(n) / (m)) : (n) / (m));
}
// Compute n mod m with positive remainder (per Knuth), even if n is negative
int kMod(int n, int m) {
    return (n < 0 ? ~(~(n) % (m)) + (m) : (n) % (m));
}

// http://docs.oracle.com/cd/B19306_01/server.102/b14200/functions004.htm
static void addMonths(struct std::tm *tm, int m) {
    // if start day is the last day of the month
    if (tm->tm_mday == daysPerMonth[isLeapYear(tm->tm_year + 1900)][tm->tm_mon]) {
        tm->tm_mday = 31;                // set to maximum
    }

    // add the months & adjust the years/months
    tm->tm_mon += m - 1;                 // convert to zero based months
    tm->tm_year += kDiv(tm->tm_mon, MONTHS_PER_YEAR);
    tm->tm_mon = kMod(tm->tm_mon, MONTHS_PER_YEAR) + 1;

    // fix the last day of month
    if (tm->tm_mday > daysPerMonth[isLeapYear(tm->tm_year + 1900)][tm->tm_mon]) {
        tm->tm_mday = daysPerMonth[isLeapYear(tm->tm_year + 1900)][tm->tm_mon];
    }
}

std::chrono::system_clock::time_point addMonthsToNow(int months) {
    time_t now = time(NULL);
    struct std::tm tm = *localtime(&now);
    addMonths(&tm, months);
    std::chrono::system_clock::time_point date = std::chrono::system_clock::from_time_t(mktime(&tm));
    return date;
}

// Return true if originalDate is after minDate
bool isValidDate(struct std::tm originalDate,
    std::chrono::system_clock::time_point &minDate) {
    std::chrono::system_clock::time_point date = std::chrono::system_clock::from_time_t(mktime(&originalDate));
    return (date > minDate);
}

std::string parseFieldFromLine(const std::string &line, const std::string &key) {
    // We're looking for a thing that looks like:
    // [key]="[value]"
    // as part of a larger string.
    // We are given [key], and want to return [value].

    // Find the start of the pattern
    std::string keyPattern = key + "=\"";
    ssize_t idx = line.find(keyPattern);

    // No match
    if (idx == -1) {
        return "";
    }

    // Find the closing quote at the end of the pattern
    size_t start = idx + keyPattern.size();

    size_t end = start;
    while (line[end] != '"') {
        end++;
    }

    // Extract [value] from the overall string and return it
    // We have (start, end); substr() requires,
    // so we must compute, (start, length).
    return line.substr(start, end-start);
}

int parseIntFromLine(const std::string &line, const std::string &key) {
    std::string strField = parseFieldFromLine(line, key);
    return atoi(strField.c_str());
}
