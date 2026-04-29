#pragma once
#include <chrono>
#include <map>
#include <vector>
#include "imgui/imgui.h"

extern ImFont* InterBold;

namespace NotifyManager
{
    enum eType {
        None,
        Info,
        Warning
    };

    class NotifyClass {
    private:
        std::string Title;
        std::string Description;
        float Duration = 0.f;
        std::chrono::steady_clock::time_point StartTime;
        eType Type = eType::None;
        bool Active = true;

    public:
        static float EaseOutCubic(float t) {
            t = (t > 1.0f) ? 1.0f : t;
            t--;
            return t * t * t + 1.0f;
        }

        void Update() {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed = now - StartTime;
            if (elapsed.count() >= Duration)
                Active = false;
        }

        void SetTitle(std::string NewTitle) { this->Title = NewTitle; }
        void SetDescription(std::string NewDesc) { this->Description = NewDesc; }
        void SetType(eType NewType) { this->Type = NewType; }
        void SetDuration(float NewDuration) { this->Duration = NewDuration; }

        std::string GetTitle() { return Title; }
        std::string GetDescription() { return Description; }
        float GetDuration() { return Duration; }
        bool IsActive() { return Active; }
        eType GetType() { return Type; }

        float GetElapsedTime() {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed = now - StartTime;
            return elapsed.count();
        }

        NotifyClass(eType Type, float Duration = 4.0f)
        {
            this->Type = Type;
            this->Duration = Duration;
            this->StartTime = std::chrono::steady_clock::now();
        }
    };

    inline std::vector<NotifyClass> NotifyList;

    inline void DeleteNotify(int Index)
    {
        NotifyList.erase(NotifyList.begin() + Index);
    }

    inline void Render()
    {
        ImVec2 ScreenSize = ImGui::GetIO().DisplaySize;
        const float Padding = 15.0f;
        const float SpacingBetween = 10.0f;
        const float MinNotifWidth = 280.0f;
        const float MaxNotifWidth = 400.0f;
        const float BaseNotifHeight = 60.0f;
        const float TextPadding = 22.0f; // Padding horizontal do texto

        for (int i = 0; i < NotifyList.size(); ++i)
        {
            auto& Notify = NotifyList[i];
            Notify.Update();

            if (!Notify.IsActive())
            {
                DeleteNotify(i);
                --i;
                continue;
            }

            float ElapsedSec = Notify.GetElapsedTime();
            float Duration = Notify.GetDuration();

            // Calcular tamanho baseado no texto (ANTES das animações para poder usar NotifWidth)
            ImVec2 titleSize = InterBold->CalcTextSizeA(InterBold->FontSize, FLT_MAX, 0.0f, Notify.GetTitle().c_str());
            ImVec2 descSize = InterBold->CalcTextSizeA(InterBold->FontSize, FLT_MAX, 0.0f, Notify.GetDescription().c_str());
            
            float textWidth = (titleSize.x > descSize.x ? titleSize.x : descSize.x) + TextPadding;
            float NotifWidth = (textWidth < MinNotifWidth) ? MinNotifWidth : ((textWidth > MaxNotifWidth) ? MaxNotifWidth : textWidth);
            
            // Calcular altura baseada no texto (pode ter múltiplas linhas)
            float textHeight = titleSize.y + descSize.y + 20.0f; // 20px de padding vertical
            float NotifHeight = (BaseNotifHeight > textHeight) ? BaseNotifHeight : textHeight;

            float AnimInDuration = 0.3f;
            float AnimOutDuration = 0.3f;

            float CurrentX = Padding; // Posição padrão
            float AnimProgress = 1.0f; // Alpha padrão

            if (ElapsedSec < AnimInDuration)
            {
                // Animação de entrada: da esquerda para a posição final
                float InProgress = NotifyClass::EaseOutCubic(ElapsedSec / AnimInDuration);
                float StartX = -NotifWidth - Padding;
                CurrentX = StartX + (Padding - StartX) * InProgress;
                AnimProgress = InProgress;
            }
            else if (ElapsedSec > Duration - AnimOutDuration)
            {
                // Animação de saída: da posição atual para a esquerda
                float OutTime = (ElapsedSec - (Duration - AnimOutDuration)) / AnimOutDuration;
                float OutProgress = NotifyClass::EaseOutCubic(OutTime);
                float EndX = -NotifWidth - Padding;
                CurrentX = Padding + (EndX - Padding) * OutProgress;
                AnimProgress = 1.0f - OutProgress;
            }

            float CurrentY = Padding + (NotifHeight + SpacingBetween) * i;

            int Alpha = static_cast<int>(255 * AnimProgress);
            Alpha = (Alpha > 255) ? 255 : (Alpha < 0) ? 0 : Alpha;

            ImU32 BgColor = IM_COL32(0x20, 0x20, 0x20, Alpha);
            ImU32 BorderColor = IM_COL32(0x2C, 0x2A, 0x2A, Alpha);
            ImU32 TitleColor = IM_COL32(215, 210, 210, Alpha);
            ImU32 MessageColor = IM_COL32(108, 105, 105, Alpha);
            ImU32 ProgressBarColor = IM_COL32(255, 255, 255, Alpha);

            ImVec2 Pos = ImVec2(CurrentX, CurrentY);
            ImVec2 NotifSize = ImVec2(NotifWidth, NotifHeight);

            auto DrawList = ImGui::GetForegroundDrawList();

            DrawList->AddRectFilled(Pos, ImVec2(Pos.x + NotifSize.x, Pos.y + NotifSize.y), BgColor, 12.0f);
            DrawList->AddRect(Pos, ImVec2(Pos.x + NotifSize.x, Pos.y + NotifSize.y), BorderColor, 12.0f, 0, 1.0f);

            ImVec2 TitlePos = ImVec2(Pos.x + 11.0f, Pos.y + 6.0f);
            ImVec2 DescPos = ImVec2(Pos.x + 11.0f, Pos.y + 26.0f);

            DrawList->AddText(LexendNormal, LexendNormal->FontSize, TitlePos, TitleColor, Notify.GetTitle().c_str());
            DrawList->AddText(LexendNormal, LexendNormal->FontSize, DescPos, MessageColor, Notify.GetDescription().c_str());

            // Barra de progresso animada diminuindo
            float Progress = 1.0f - (ElapsedSec / Duration);
            Progress = (Progress > 1.0f) ? 1.0f : (Progress < 0.0f) ? 0.0f : Progress;

            float BarWidth = (NotifWidth - 20) * Progress;
            float BarHeight = 5.0f;
            ImVec2 BarPos = ImVec2(Pos.x + 10, Pos.y + NotifHeight - 12);

            DrawList->AddRectFilled(BarPos, ImVec2(BarPos.x + BarWidth, BarPos.y + BarHeight), ProgressBarColor, 10.0f);
        }
    }

    inline void Send(std::string Description, float Duration = 4.0f)
    {
        NotifyManager::NotifyClass Notify(NotifyManager::eType::Info, Duration);
        Notify.SetTitle("Notify - Sharp Menu");
        Notify.SetDescription(Description);
        NotifyList.push_back(Notify);
    }
}
