#include "stdafxClientFramework.h"
#include "LocalPlayer.h"
#include "GlobalInterfaces.h"

using namespace DirectX;

void LocalPlayer::Update(float deltaTime)
{
<<<<<<< Updated upstream
    // GameFramework¿¡¼­ ÇÃ·¹ÀÌ¾î ¾÷µ¥ÀÌÆ® ÄÚµå¸¦ ¿©±â·Î ¿Å±è
  // ÇÃ·¹ÀÌ¾î ¹Ù¶óº¸´Â ¹æÇâ º¤ÅÍ °è»ê
    float forwardX = sinf(m_cameraYaw);
    float forwardZ = cosf(m_cameraYaw);

    // ¿ìÃø º¤ÅÍ °è»ê
=======
    // GameFrameworkì—ì„œ í”Œë ˆì´ì–´ ì—…ë°ì´íŠ¸ ì½”ë“œë¥¼ ì—¬ê¸°ë¡œ ì˜®ê¹€
  // í”Œë ˆì´ì–´ ë°”ë¼ë³´ëŠ” ë°©í–¥ ë²¡í„° ê³„ì‚°
    float forwardX = sinf(m_cameraYaw);
    float forwardZ = cosf(m_cameraYaw);

    // ìš°ì¸¡ ë²¡í„° ê³„ì‚°
>>>>>>> Stashed changes
    float rightX = sinf(m_cameraYaw + XM_PIDIV2);
    float rightZ = cosf(m_cameraYaw + XM_PIDIV2);

    float moveSpeed = m_playerSpeed * deltaTime;

<<<<<<< Updated upstream
    // WASD ÀÔ·Â Ã³¸®
    if(Globals::Input().GetInput('W'))  // ÀüÁø
=======
    // WASD ìž…ë ¥ ì²˜ë¦¬
    if (Globals::Input().GetInput('W'))  // ì „ì§„
>>>>>>> Stashed changes
    {
        m_x += forwardX * moveSpeed;
        m_z += forwardZ * moveSpeed;
        sendFlag = true;
    }
<<<<<<< Updated upstream
    if(Globals::Input().GetInput('S'))  // ÈÄÁø
=======
    if (Globals::Input().GetInput('S'))  // í›„ì§„
>>>>>>> Stashed changes
    {
        m_x -= forwardX * moveSpeed;
        m_z -= forwardZ * moveSpeed;
        sendFlag = true;
    }
<<<<<<< Updated upstream
    if(Globals::Input().GetInput('A'))  // ÁÂÃø ÀÌµ¿
=======
    if (Globals::Input().GetInput('A'))  // ì¢Œì¸¡ ì´ë™
>>>>>>> Stashed changes
    {
        m_x -= rightX * moveSpeed;
        m_z -= rightZ * moveSpeed;
        sendFlag = true;
    }
<<<<<<< Updated upstream
    if(Globals::Input().GetInput('D'))  // ¿ìÃø ÀÌµ¿
=======
    if (Globals::Input().GetInput('D'))  // ìš°ì¸¡ ì´ë™
>>>>>>> Stashed changes
    {
        m_x += rightX * moveSpeed;
        m_z += rightZ * moveSpeed;
        sendFlag = true;
    }

<<<<<<< Updated upstream
    // ¼öÁ÷ ÀÌµ¿ (H/L Å°)
    if(Globals::Input().GetInput('H')) {
        m_y -= moveSpeed;  // ¾Æ·¡·Î
        sendFlag = true;
    }
    if(Globals::Input().GetInput('L')) {
        m_y += moveSpeed;  // À§·Î
        sendFlag = true;
    }

    // À§Ä¡ µð¹ö±ë
=======
    // ìˆ˜ì§ ì´ë™ (H/L í‚¤)
    if (Globals::Input().GetInput('H')) {
        m_y -= moveSpeed;  // ì•„ëž˜ë¡œ
        sendFlag = true;
    }
    if (Globals::Input().GetInput('L')) {
        m_y += moveSpeed;  // ìœ„ë¡œ
        sendFlag = true;
    }

    // ìœ„ì¹˜ ë””ë²„ê¹…
>>>>>>> Stashed changes
    static float lastX = 0, lastY = 1, lastZ = 0;
    if (m_x != lastX || m_z != lastZ) {
        DEBUG_LOG_FMT("Player Position: ({:.2f}, {:.2f}, {:.2f})\n",
            m_x, m_y, m_z);
        lastX = m_x; lastY = m_y; lastZ = m_z;
    }

<<<<<<< Updated upstream
    // ===== ¸¶¿ì½º·Î Ä«¸Þ¶ó ÀÌµ¿ =====
    bool isLeftButtonPressed = Globals::Input().GetInput(VK_LBUTTON);
    // ÇöÀç ¸¶¿ì½º À§Ä¡
=======
    // ===== ë§ˆìš°ìŠ¤ë¡œ ì¹´ë©”ë¼ ì´ë™ =====
    bool isLeftButtonPressed = Globals::Input().GetInput(VK_LBUTTON);
    // í˜„ìž¬ ë§ˆìš°ìŠ¤ ìœ„ì¹˜
>>>>>>> Stashed changes
    auto mousePos = Globals::Input().GetMousePosition();

    if (isLeftButtonPressed) {
        if (!m_isMouseDragging) {
            m_isMouseDragging = true;
<<<<<<< Updated upstream
            m_lastMouseX = mousePos.x;  // ½ÃÀÛ À§Ä¡ ÀúÀå
=======
            m_lastMouseX = mousePos.x;  // ì‹œìž‘ ìœ„ì¹˜ ì €ìž¥
>>>>>>> Stashed changes
            m_lastMouseY = mousePos.y;
            DEBUG_LOG_FMT("Camera drag started at ({:.1f}, {:.1f})\n", mousePos.x, mousePos.y);
        }
        else {
<<<<<<< Updated upstream
            // ¿òÁ÷ÀÓ °¨Áö
            float deltaX = mousePos.x - m_lastMouseX;
            float deltaY = mousePos.y - m_lastMouseY;

            if(abs(deltaX) > 0.1f || abs(deltaY) > 0.1f) {
                // Ä«¸Þ¶ó È¸Àü ¾÷µ¥ÀÌÆ®
                m_cameraYaw += deltaX * m_mouseSensitivity;
                m_cameraPitch += deltaY * m_mouseSensitivity;

                // Pitch Á¦ÇÑ (À§¾Æ·¡ È¸Àü Á¦ÇÑ)
                m_cameraPitch = std::clamp(m_cameraPitch, -1.5f, 1.5f);

                //µð¹ö±ë
=======
            // ì›€ì§ìž„ ê°ì§€
            float deltaX = mousePos.x - m_lastMouseX;
            float deltaY = mousePos.y - m_lastMouseY;

            if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f) {
                // ì¹´ë©”ë¼ íšŒì „ ì—…ë°ì´íŠ¸
                m_cameraYaw += deltaX * m_mouseSensitivity;
                m_cameraPitch += deltaY * m_mouseSensitivity;

                // Pitch ì œí•œ (ìœ„ì•„ëž˜ íšŒì „ ì œí•œ)
                m_cameraPitch = std::clamp(m_cameraPitch, -1.5f, 1.5f);

                //ë””ë²„ê¹…
>>>>>>> Stashed changes
                DEBUG_LOG_FMT("Camera rotating - Delta({:.1f}, {:.1f}) Yaw: {:.2f}, Pitch: {:.2f}\n",
                    deltaX, deltaY, m_cameraYaw, m_cameraPitch);
            }

            m_lastMouseX = mousePos.x;
            m_lastMouseY = mousePos.y;
        }
    }
    else {
<<<<<<< Updated upstream
        if(m_isMouseDragging) {
            // µå·¡±× Á¾·á
=======
        if (m_isMouseDragging) {
            // ë“œëž˜ê·¸ ì¢…ë£Œ
>>>>>>> Stashed changes
            m_isMouseDragging = false;
            DEBUG_LOG_FMT("Camera drag ended\n");
        }
    }

<<<<<<< Updated upstream
    //¸¶¿ì½º ÈÙ·Î ÁÜÀÎ¾Æ¿ô
=======
    //ë§ˆìš°ìŠ¤ íœ ë¡œ ì¤Œì¸ì•„ì›ƒ
>>>>>>> Stashed changes
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