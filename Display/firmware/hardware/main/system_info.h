#pragma once
struct PanelSystemInfo {
    char hardware[1024]={};
    char runtime[1536]={};
};
void readPanelSystemInfo(PanelSystemInfo& out);
