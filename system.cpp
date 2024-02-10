#include "header.h"

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // unix system
    // for windoes maybe we must add the following
    // __cpuid(regs, 0);
    // regs is the array of 4 positions
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
string GetOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

bool OnlyDigits(char *name) {
    char x = name[0];
    int i = 0;

    while (x != '\0') {
        if (x < '0' || x > '9')
            return false;
        i++;
        x = name[i];
    }

    return true;
}

int NumberOfRunningProcesses() {
    DIR* dir = opendir("/proc");

    int count = 0;

    while(dirent* dirp = readdir(dir))
    {
        // is this a directory?
        if(dirp->d_type != DT_DIR)
            continue;

        // is every character of the name a digit?
        if(!OnlyDigits(dirp->d_name))
            continue;

        // digit only named directory, must be a process
        ++count;
    }

    closedir(dir);

    return count;
}

string TopSystemInfo() {
    string res;

    char hostname[HOST_NAME_MAX];
    char username[LOGIN_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    getlogin_r(username, LOGIN_NAME_MAX);

    res = "Operating system : " + GetOsName();
    res = res + "\nComputer name : " + hostname;
    res = res + "\nUser logged in : " + username;
    res = res + "\nNumber of running processes : " + to_string(NumberOfRunningProcesses());
    res = res + "\nCPU : " + CPUinfo() + "\n";

    return res;
}

const int maxGraph = 120;
int tabOption = 0;
int fps = 1;
float scale = 100.0;
uint64_t lastValueTime = 0;
std::list <double> valueList;
float labelValue;
string tabs[] = {"CPU", "Temp", "Fan", "Power"};
bool animate = true;

float MaxGraphValue() {
    switch (tabOption) {
        case 0:
        return 100.0;
        case 1:
        return 200.0;
        case 2:
        return 5000.0;
        break;
        case 3:
        break;
    }
    return 0.0;
}


void BuildSystemWindow(ImVec2 size) {
    string topInfo = TopSystemInfo();
    float lineHeight = ImGui::CalcTextSize("\n").y;
    ImGui::Text(&topInfo[0]);

    ImGui::Separator();

    int newtabOption = GenerateTabsName(tabs, 3, tabOption);

    if (newtabOption != tabOption){
        tabOption = newtabOption;
        valueList.clear();
        scale = MaxGraphValue();
        labelValue = 0.0;
    }

    ImGui::SliderInt("FPS", &fps, 1, 30);
    ImGui::SliderFloat("Max scale", &scale, 0.0, MaxGraphValue());
    ImGui::Checkbox("Animate", &animate);

    double newValue;
    double timeDiff = double(timeSinceEpochMillisec() -  lastValueTime) / 1000.0;

    if (animate && timeDiff > 1.0 / double(fps)) {
        switch (tabOption) {
            case 0:
            newValue = CalculateCpuUsage();
            break;
            case 1:
            newValue = GetThermalValue();
            break;
            case 2:
            newValue = GetFanValue();
            break;
            case 3:
            newValue = 0.0f;
            break;
            default:
            newValue = 0.0f;
        }
        labelValue = newValue;
        lastValueTime = timeSinceEpochMillisec();
        valueList.push_back(newValue);
        if (valueList.size() > maxGraph) {
            valueList.pop_front();
        }
    }

    tabOption = newtabOption;
    float valueArr[maxGraph];
    int n = valueList.size();

    for (int i = 0; i < maxGraph - n; i++) {
        valueArr[i] = 0.0;
    }

    int i = maxGraph - n;
    for (auto it = valueList.begin(); it != valueList.end(); it++) {
        valueArr[i] = *it;
        i++;
    }

    string label;
    if (labelValue < 0.0f) {
        label = "Failed to find information.";
    } else {
        label = tabs[tabOption]; +"\n" + to_string(labelValue);
    }
    

    switch (tabOption) {
        case 0:
        label = label + " (%)\n" + to_string(labelValue);
        break;
        case 1:
        label = label + " (°C)\n" + to_string(int(labelValue));
        break;
        case 2:
        label = label + (" (rpm)\n") + to_string(int(labelValue));
        break;
        case 3:
        // label = label + 
        break;
    }

    ImGui::PlotLines(&label[0], valueArr, maxGraph, 0, NULL, 0.0f, scale, ImVec2(size.x - 110.0f, size.y - 10.0f*lineHeight));
}

void InitSystemValues() {
    lastValueTime = timeSinceEpochMillisec();
    InitSystemUtilsValues();
}