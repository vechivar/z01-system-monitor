#include "header.h"

unordered_map<string, Proc> runningProcesses; // map des processus en cours, avec leur pid comme clé
uint64_t lastProcUpdate = 0;    // moment de la dernière mise à jour des processus
uint64_t updateDelta = 5000;    // temps en ms entre deux mises à jour de la liste des processus
double totalMemory; // taille de la mémoire ram
static double pageSize = double(sysconf(_SC_PAGESIZE));

// transforme un nombre de KB en une string de format convenable
string KbToGoodFormat(double x) {
    stringstream res;
    int unit = 0;

    while (x > 999) {
        unit++;
        x = x/1024;
    }

    res << fixed << setprecision(2) << x;
    switch (unit) {
        case 0:
        res << "KB";
        break;
        case 1:
        res << "MB";
        break;
        case 2:
        res << "GB";
        break;
        default:
        cout << "problem";
    }

    return res.str();
}

// pour avoir des chiffres conformes à ifconfig dans la partie réseau...
string KbToGoodFormat_k(double x) {
    stringstream res;
    int unit = 0;

    while (x > 999) {
        unit++;
        x = x/1000;
    }

    res << fixed << setprecision(2) << x;
    switch (unit) {
        case 0:
        res << "KB";
        break;
        case 1:
        res << "MB";
        break;
        case 2:
        res << "GB";
        break;
        default:
        cout << "problem";
    }

    return res.str();
}

// affiche une donnée d'utilisation sous forme de barre
void PrintBar(double used, double total, string text, float barWidth) {
    string label = text + " : " + KbToGoodFormat(total);
    ImGui::Text(&label[0]);
    if (used == 0.0 || total == 0.0) {
        ImGui::ProgressBar(0.0f, ImVec2(barWidth, 0.0f));
    } else {
        ImGui::ProgressBar(used/total, ImVec2(barWidth, 0.0f));
    }
    ImGui::SameLine();
    label = "Used : " + KbToGoodFormat(used);
    ImGui::Text(&label[0]);
}

// affiche toutes les barres dans l'interface
void PrintMemoryBars(float barWidth) {
    double value = 0.0;
    double total = 0.0;
    double used = 0.0;
    double totalswap = 0.0;
    double usedswap = 0.0;
    char type[128];
    char unit[128];

    // lecture du fichier
    FILE* f = fopen("/proc/meminfo", "r");
    if (f == NULL) {
        ImGui::Text("Couldn't find file /proc/meminfo");
        ImGui::Text("No information on memory usage was built.");
        return;
    }
    while (fscanf(f, "%s %lf %s", type, &value, unit) != EOF) {
        // mémoire totale
        if (string(type) == "MemTotal:") {
            totalMemory = double(value);
            total = value;
            used += value;
        }
        // mémoire utilisée = total - cached - buffers - memfree - sreclaimable
        if (string(type) == "Cached:" || string(type) =="Buffers:"|| string(type) =="MemFree:" || string(type) =="SReclaimable:") {
            used -= value;
        }
        // mémoire swap totale
        if (string(type) == "SwapTotal:") {
            totalswap = value;
            usedswap += value;
        }
        // mémoire swap utilisée = totale - free
        if (string(type) == "SwapFree:") {
            usedswap -= value;
        }
    }
    fclose(f);

    // informations du disque
    struct statvfs d;
    statvfs("/", &d);

    // affichage des barres
    PrintBar(used, total, "Physical memory (RAM)", barWidth);
    PrintBar(usedswap, totalswap, "Virtual memory (SWAP)", barWidth);
    PrintBar(d.f_bsize*(d.f_blocks - d.f_bfree) / 1024.0, (d.f_bsize * d.f_blocks) / 1024.0, "Disk size", barWidth);
}

// construit la structure correspondant au processus du pid donné
Proc BuildProcess(string pid) {
    Proc baseProc = Proc{pid:pid, valid:true};
    double newCpu = 0.0;
    double rss;

    string path = "/proc/" + pid + "/stat";
    FILE* f = fopen(&path[0], "r");
    if (f == NULL) {
        // impossible d'ouvrir le fichier, le processus s'est déjà terminé.
        baseProc.valid = false;
        return baseProc;
    }

    char value[128] = "";
    string strValue;
    uint64_t now = timeSinceEpochMillisec();

    // lecture du fichier, on récupère les valeurs utiles
    for (int i = 0; i < 24; i ++) {
        fscanf(f, "%s", value);
        if (i==1) {
            // nom du processus, entre parenthèses.
            // attention : il peut contenir des espaces
            strValue = string(value);
            strValue.erase(0,1);
            while (strValue[strValue.size() - 1] != ')') {
                fscanf(f, "%s", value);
                strValue += " " + string(value);
            }
            strValue.erase(strValue.size() - 1, 1);
            baseProc.name = strValue;
        }
        // état du processus
        if (i==2) {
            baseProc.state = string(value);
        }
        // total du temps d'occupation d'un processeur par le process depuis sa création.
        if (i == 13 || i == 14) {
            newCpu += stod(string(value));
        }
        // mémoire utilisée
        if (i == 23) {
            rss = stod(string(value)) * pageSize;
        }
    }
    fclose(f);

    newCpu = newCpu /  double(sysconf(_SC_CLK_TCK));

    double cpuUsage;
    stringstream cpuUsageStr, memUsageStr;

    cpuUsageStr << fixed << setprecision(1); 

    // calcul du taux d'utilisation du CPU.
    // (total temps utilisation maintenant) - (total temps utilisation avant) / (différence de temps maintenant-avant)
    if (runningProcesses.find(pid) != runningProcesses.end()) {
        cpuUsage = (newCpu - runningProcesses[pid].totalTime) * 100000.0 / double(now - runningProcesses[pid].lastUpdate);
        cpuUsageStr << cpuUsage;
        baseProc.cpuUsage = cpuUsageStr.str();
    } else {
        // le process a été créé après la dernière actualisation. on ne peut pas calculer le taux d'utilisation.
        baseProc.cpuUsage = "0.0";
    }

    memUsageStr << fixed << setprecision(1);
    memUsageStr << 100.0 * rss/(totalMemory * 1024);
    baseProc.memUsage = memUsageStr.str();

    baseProc.lastUpdate = now;
    baseProc.totalTime = newCpu;

    return baseProc;
}

// Mise à jour de la liste des process
void UpdateProcesses() {
    DIR* dir = opendir("/proc");
    string pid;
    Proc proc;

    unordered_map<string, Proc> updatedProcesses;

    // parcours des fichiers contenant les informations
    while(dirent* dirp = readdir(dir)) {
        if(dirp->d_type == DT_DIR && OnlyDigits(dirp->d_name)) {
            pid = string(dirp->d_name);
            proc = BuildProcess(pid);
            if (proc.valid) {
                updatedProcesses.insert({pid, proc});
            }
        }
    }
    runningProcesses = updatedProcesses;

    closedir(dir);
}

// affichage des process dans l'interface
void PrintProcesses() {
    char str[128] = "";
    ImGui::InputText("Search process", str, 128);
    string lookFor = string(str);

    DIR* dir = opendir("/proc");
    string pname = "";

    list <Proc> toPrint;
    int maxNameSize = 0;
    int size;

    // filtrage des process avec la barre de recherche
    for (const pair<string, Proc> it : runningProcesses) {
        if (lookFor == "" || it.second.name.find(lookFor) != string::npos || it.second.pid.find(lookFor) != string::npos) {
            size = it.second.name.size();
            if (size > maxNameSize) {
                maxNameSize = size;
            }
            toPrint.push_back(it.second);
        }
    }

    closedir(dir);

    //construction du tableau
    if (ImGui::BeginTable("Result : ", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_ColumnsWidthStretch))
        {
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 8.0f);
            ImGui::TableSetupColumn("NAME", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("STATE", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 5.5f);
            ImGui::TableSetupColumn("CPU %", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 8.0f);
            ImGui::TableSetupColumn("MEM %", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 8.0f);
            ImGui::TableHeadersRow();
            for (auto it = toPrint.begin(); it != toPrint.end(); it++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(&it->pid[0]);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(&it->name[0]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text(&it->state[0]);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text(&it->cpuUsage[0]);
                ImGui::TableSetColumnIndex(4);
                ImGui::Text(&it->memUsage[0]);
            }
            ImGui::EndTable();
        }
}

// fonction principale de la fenêtre
void BuildMemWindow(ImVec2 size) {
    PrintMemoryBars(size.x/2.0);
    ImGui::Separator();

    uint64_t now = timeSinceEpochMillisec();
    if (now - lastProcUpdate > updateDelta) {
        UpdateProcesses();
        lastProcUpdate = now;
    }

    if (ImGui::TreeNode("Process table")) {
        PrintProcesses();
        ImGui::TreePop();
    }
}
