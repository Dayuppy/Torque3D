#include "console/engineAPI.h"
#include <ctime>
#include <vector>
#include <string>

using namespace Torque;

// simple pad helper
static String _pad(int v, bool zero) {
    char b[8];
    dSprintf(b, sizeof(b), zero ? "%02d" : "%d", v);
    return String(b);
}

// replace every occurrence of 's' in 'in' with 'r'
static String _replaceAll(String in, const String& s, const String& r) {
    S32 pos = in.find(s);
    while (pos != -1) {
        in = in.substr(0, pos) + r + in.substr(pos + s.length());
        pos = in.find(s, pos + r.length());
    }
    return in;
}

DefineEngineFunction(formatTimeString, const char*, (const char* pattern), ,
    "@brief Format the current *local* time using compact, brace-free tokens.\n\n"
    "Supported tokens:\n"
    "  yyyy → four-digit year    yy → two-digit\n"
    "  DD   → full weekday name   D  → short\n"
    "  MM   → full month name     M  → short\n"
    "  mm   → zero-padded month   m  → no-pad month\n"
    "  dd   → zero-padded day     d  → no-pad day\n"
    "  HH   → zero-padded 24h     H  → no-pad 24h\n"
    "  hh   → zero-padded 12h     h  → no-pad 12h\n"
    "  nn   → zero-padded minute  n  → no-pad minute\n"
    "  ss   → zero-padded second  s  → no-pad second\n"
    "  A    → \"AM\"              a  → \"am\"\n"
    "  Z    → literal \"Z\" (unchanged)\n\n"
    "@param pattern A format like \"hh:nn a, mm-dd\" or \"yyyy-MM-dd HH:nn:ss\".\n"
    "@return The formatted local time string.\n"
)
{
    // 1) get local time from CRT
    time_t raw = time(nullptr);
    struct tm localTm;
#if defined(_WIN32)
    localtime_s(&localTm, &raw);
#else
    localtime_r(&raw, &localTm);
#endif

    int year = localTm.tm_year + 1900;
    int mon = localTm.tm_mon + 1;
    int day = localTm.tm_mday;
    int hour = localTm.tm_hour;
    int minute = localTm.tm_min;
    int second = localTm.tm_sec;
    int wday = localTm.tm_wday; // 0=Sunday

    // name tables
    static const char* wks[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
    static const char* wkf[] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };
    static const char* mos[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
    static const char* mof[] = {
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };

    // 2) ordered replacements
    std::vector<std::pair<String, String>> reps = {
        { "yyyy", String::ToString(year) },
        { "yy",   _pad(year % 100, true) },
        { "DD",   wkf[wday] },   { "D", wks[wday] },
        { "MM",   mof[mon - 1] },  { "M", mos[mon - 1] },
        { "mm",   _pad(mon, true) }, { "m", _pad(mon, false) },
        { "dd",   _pad(day, true) }, { "d", _pad(day, false) },
        { "HH",   _pad(hour, true) },{ "H", _pad(hour, false) },
        { "hh",   _pad((hour % 12 ? hour % 12 : 12), true) },
        { "h",    _pad((hour % 12 ? hour % 12 : 12), false) },
        { "nn",   _pad(minute, true) },{ "n", _pad(minute, false) },
        { "ss",   _pad(second, true) },{ "s", _pad(second, false) },
        { "Z",    "Z" },
        { "A",    hour < 12 ? "AM" : "PM" },
        { "a",    hour < 12 ? "am" : "pm" }
    };

    String out(pattern);
    for (auto& kv : reps)
        out = _replaceAll(out, kv.first, kv.second);

    return Con::getReturnBuffer(out);
}