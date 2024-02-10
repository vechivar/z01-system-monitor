#include "header.h"

struct GraphEntry {
    double value;
    uint64_t time;
};

// CPU
int cpuCount = 8;
uint64_t cpuSmoothFactor = 1500;
std::list <GraphEntry> cpuEntries;

// thermal
string thermalPath;
std::list <GraphEntry> thermalEntries;


uint64_t timeSinceEpochMillisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// récupère le total idletime dans le fichier /proc/uptime
double GetCpuIdleValue() {
    float uptime, idletime;
    FILE* f = fopen("/proc/uptime", "r");
    if (f== NULL) {
        return -1.0f;
    }
    fscanf(f, "%f %f", &uptime, &idletime);
    fclose(f);

    return double(idletime);
}

int InitCpuValues() {
    bool flag = true;
    int cpuCount = -1;
    string line;

    std::ifstream file("/proc/stat");

    while (flag) {  
        getline(file, line);
        if (line.substr(0, 3) == "cpu") {
            cpuCount++;
        } else {
            flag = false;
        }
    }
    file.close();

    cpuEntries.push_front(GraphEntry{GetCpuIdleValue(), timeSinceEpochMillisec()});

   return 0;
}


double CalculateCpuUsage() {
    double newCpu = GetCpuIdleValue();
    uint64_t newTime = timeSinceEpochMillisec();

    while (cpuEntries.size() > 1 && newTime - cpuEntries.back().time > cpuSmoothFactor) {
        cpuEntries.pop_back();
    }

    GraphEntry last = cpuEntries.back();

    double idleDiff = (newCpu - last.value) / double(cpuCount);
    double timeDiff = double(newTime - last.time) / 1000.0;    

    cpuEntries.push_front(GraphEntry{newCpu, newTime});

    return (1.0 - (idleDiff / timeDiff)) * 100.0;
}

void InitThermalValues() {
    thermalPath = "";
    string baseDir = "/sys/class/thermal/thermal_zone";
    int i = 0;
    bool flag = true;
    std::ifstream f;
    string line;
    string searchPath;

    while (flag) {
        searchPath = baseDir + to_string(i) + "/type";
        f.open(searchPath);
        if (f.is_open()) {
            getline(f, line);
            f.close();
            if (line == "acpitz") {
                thermalPath = baseDir + to_string(i) + "/temp";
                flag = false;
            }
            i++;
        } else {
            flag = false;
        }
    }
}

float GetThermalValue() {
    float temp;
    FILE* f = fopen(&thermalPath[0], "r");
    if (f == NULL) {
        return -1000.0;
    }
    fscanf(f, "%f", &temp);
    fclose(f);
    return temp / 1000.0;
}

float GetFanValue() {
    float spd;
    FILE* f = fopen("/sys/class/hwmon/hwmon4/fan1_input", "r");
    if (f == NULL) {
        return -1.0;
    }
    fscanf(f, "%f", &spd);
    fclose(f);
    return spd;
}

void InitSystemUtilsValues() {
    InitCpuValues();
    InitThermalValues();
}

int GenerateTabsName(string* names, int tabsTotal, int lastOption) {
    int tabOption = lastOption;
    for (int i=0; i < tabsTotal; i++) {
        if (ImGui::Button(&names[i][0])) {
            tabOption = i;
        }
        if (i != tabsTotal - 1) {
            ImGui::SameLine();  
        }
    }

    return tabOption;
}