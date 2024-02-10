#include "header.h"

int networksTotal = 0;
string rxHeaders[9] = {"interface", "bytes", "packets", "errs", "drop", "fifo", "frame", "compressed", "multicast"};
string txHeaders[9] = {"interface", "bytes", "packets", "errs", "drop", "fifo", "colls", "carrier", "compressed"};
int networkTabOption = 0;


void BuildIp4Window() {
    struct ifaddrs* ptr_ifaddrs = nullptr;
    getifaddrs(&ptr_ifaddrs);

    ImGui::Text("Ip4 network :");
    string text;

    networksTotal = 0;

    for (ifaddrs* ptr_entry = ptr_ifaddrs; ptr_entry != nullptr; ptr_entry = ptr_entry->ifa_next) {
        if (ptr_entry->ifa_addr->sa_family == AF_INET) {
            char buffer[INET_ADDRSTRLEN] = {0, };
            inet_ntop(AF_INET, &((struct sockaddr_in*)(ptr_entry->ifa_addr))->sin_addr, buffer, INET_ADDRSTRLEN);
            text = "   " + string(ptr_entry->ifa_name) + " : " + string(buffer);
            ImGui::Text(&text[0]);
            networksTotal++;
        }
    }
    free(ptr_ifaddrs);
}

void BuildNetworkTableWindow(float barWidth) {
    char buffer[128] = "";
    char x = 'a';
    string rxValues[networksTotal][9];
    string txValues[networksTotal][8];

    FILE* f = fopen("/proc/net/dev", "r");
    if (f == NULL) {
        ImGui::Text("Couldn't find file /proc/net/dev");
        ImGui::Text("No information on network usage was built.");
    }

    for (int i = 0; i < 2; i++) {
        fscanf(f, "%c", &x);
        while (x != '\n') {
            fscanf(f, "%c", &x);
        }
    }

    for (int i = 0; i < networksTotal; i++) {
        for (int j = 0; j < 9; j++) {
            fscanf(f, "%s", buffer);
            rxValues[i][j] = string(buffer);
        }
        for (int j = 0; j < 8; j++) {
            fscanf(f, "%s", buffer);
            txValues[i][j] = string(buffer);
        }
    }

    fclose(f);
    
    if (ImGui::TreeNode("Network table")) {
        if (ImGui::TreeNode("RX")) {
            if (ImGui::BeginTable("RX", 9, ImGuiTableFlags_Borders)) {
                for (int i = 0; i < 9; i++) {
                    ImGui::TableSetupColumn(&rxHeaders[i][0]);
                }
                ImGui::TableHeadersRow();
                for (int i =0; i < networksTotal; i++) {
                    ImGui::TableNextRow();
                    for (int j = 0; j < 9; j++) {
                        ImGui::TableSetColumnIndex(j);
                        ImGui::Text(&rxValues[i][j][0]);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("TX")) {
            if (ImGui::BeginTable("TX", 9, ImGuiTableFlags_Borders)) {
                for (int i = 0; i < 9; i++) {
                    ImGui::TableSetupColumn(&txHeaders[i][0]);
                }
                ImGui::TableHeadersRow();
                for (int i = 0; i < networksTotal; i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(&rxValues[i][0][0]);
                    for (int j = 0; j < 8; j++) {
                        ImGui::TableSetColumnIndex(j+1);
                        ImGui::Text(&txValues[i][j][0]);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    double barsToPrint[networksTotal];

    if (ImGui::Button("Receive (RX)")) {
        networkTabOption = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Transmeit (TX)")) {
        networkTabOption = 1;
    }

    switch (networkTabOption) {
        case 0:
        for (int i = 0; i < networksTotal; i++) {
            barsToPrint[i] = stod(rxValues[i][1]);
        }
        break;
        case 1:
        for (int i = 0; i < networksTotal; i++) {
            barsToPrint[i] = stod(txValues[i][0]);
        }
        break;
    }

    for (int i = 0; i < networksTotal; i++) {
        ImGui::Text(&rxValues[i][0][0]);
        // ImGui::ProgressBar(barsToPrint[i]/2147483648, ImVec2(barWidth, 0.0f));
        ImGui::ProgressBar(barsToPrint[i]/2000000000, ImVec2(barWidth, 0.0f));
        ImGui::SameLine();
        ImGui::Text(&KbToGoodFormat_k(barsToPrint[i]/1000)[0]);
        ImGui::Text("0GB");
        ImGui::SameLine(barWidth - 15.0);
        ImGui::Text("2GB");
    }
}

void BuildNetworkWindow(ImVec2 size) {
    BuildIp4Window();
    ImGui::Separator();
    BuildNetworkTableWindow(size.x/2);
}