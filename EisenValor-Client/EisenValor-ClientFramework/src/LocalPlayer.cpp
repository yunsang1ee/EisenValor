#include "stdafxClientFramework.h"
#include "LocalPlayer.h"
#include "GlobalInterfaces.h"

using namespace DirectX;

void LocalPlayer::Update(float deltaTime)
{
    // GameFramework���� �÷��̾� ������Ʈ �ڵ带 ����� �ű�
  // �÷��̾� �ٶ󺸴� ���� ���� ���
    float forwardX = sinf(m_cameraYaw);
    float forwardZ = cosf(m_cameraYaw);

    // ���� ���� ���
    float rightX = sinf(m_cameraYaw + XM_PIDIV2);
    float rightZ = cosf(m_cameraYaw + XM_PIDIV2);

    float moveSpeed = m_playerSpeed * deltaTime;

    // WASD �Է� ó��
    if (Globals::Input().GetInput('W'))  // ����
    {
        m_x += forwardX * moveSpeed;
        m_z += forwardZ * moveSpeed;
        sendFlag = true;
    }
    if (Globals::Input().GetInput('S'))  // ����
    {
        m_x -= forwardX * moveSpeed;
        m_z -= forwardZ * moveSpeed;
        sendFlag = true;
    }
    if (Globals::Input().GetInput('A'))  // ���� �̵�
    {
        m_x -= rightX * moveSpeed;
        m_z -= rightZ * moveSpeed;
        sendFlag = true;
    }
    if (Globals::Input().GetInput('D'))  // ���� �̵�
    {
        m_x += rightX * moveSpeed;
        m_z += rightZ * moveSpeed;
        sendFlag = true;
    }

    // ���� �̵� (H/L Ű)
    if (Globals::Input().GetInput('H')) {
        m_y -= moveSpeed;  // �Ʒ���
        sendFlag = true;
    }
    if (Globals::Input().GetInput('L')) {
        m_y += moveSpeed;  // ����
        sendFlag = true;
    }

    // ��ġ �����
    static float lastX = 0, lastY = 1, lastZ = 0;
    if (m_x != lastX || m_z != lastZ) {
        DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n",
            m_x, m_y, m_z);
        lastX = m_x; lastY = m_y; lastZ = m_z;
    }

    // ===== ���콺�� ī�޶� �̵� =====
    bool isLeftButtonPressed = Globals::Input().GetInput(VK_LBUTTON);
    // ���� ���콺 ��ġ
    auto mousePos = Globals::Input().GetMousePosition();

    if (isLeftButtonPressed) {
        if (!m_isMouseDragging) {
            m_isMouseDragging = true;
            m_lastMouseX = mousePos.x;  // ���� ��ġ ����
            m_lastMouseY = mousePos.y;
            DEBUG_LOG_FMT("Camera drag started at ({:.1f}, {:.1f})\n", mousePos.x, mousePos.y);
        }
        else {
            // ������ ����
            float deltaX = mousePos.x - m_lastMouseX;
            float deltaY = mousePos.y - m_lastMouseY;

            if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f) {
                // ī�޶� ȸ�� ������Ʈ
                m_cameraYaw += deltaX * m_mouseSensitivity;
                m_cameraPitch += deltaY * m_mouseSensitivity;

                // Pitch ���� (���Ʒ� ȸ�� ����)
                m_cameraPitch = std::clamp(m_cameraPitch, -1.5f, 1.5f);

                //�����
                DEBUG_LOG_FMT("Camera rotating - Delta({:.1f}, {:.1f}) Yaw: {:.2f}, Pitch: {:.2f}\n",
                    deltaX, deltaY, m_cameraYaw, m_cameraPitch);
            }

            m_lastMouseX = mousePos.x;
            m_lastMouseY = mousePos.y;
        }
    }
    else {
        if (m_isMouseDragging) {
            // �巡�� ����
            m_isMouseDragging = false;
            DEBUG_LOG_FMT("Camera drag ended\n");
        }
    }

    //���콺 �ٷ� ���ξƿ�
    int wheelDelta = Globals::Input().GetWheelScroll();
    if (wheelDelta != 0) {
        m_cameraDistance -= wheelDelta * 0.001f;
        m_cameraDistance = std::clamp(m_cameraDistance, 5.0f, 30.0f);
    }

    if (sendFlag) {
        const FB_STRUCTS::Vec3 pos{ m_x, m_y, m_z };
        const FB_STRUCTS::Vec3 rot{ 0.f, m_yaw, 0.f };
        const auto packetData = NetBridge::ServerPacketHandler::Make_CS_PLAYER_MOVE_PACKET(&pos, &rot);
        const auto packetBuffer = NetBridge::ServerPacketHandler::MakeSendBuffer(PACKET_TYPE::CS_PLAYER_MOVE, packetData);
        MANAGER(NetBridge::NetworkManager)->Send(packetBuffer);
        sendFlag = false;
    }
}