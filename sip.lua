require "util"
function worker()
    local password = "1234"
    local method = "REGISTER"
    local user = "1000"
    while true do
        local reg = [[REGISTER sip:16692370001.zoomcloudpbx.com;transport=TCP SIP/2.0
Via: SIP/2.0/TCP 10.100.125.18:9090;branch=z9hG4bK-3899-1-0;rport
From:<sip:]]..user..[[@16692370001.zoomcloudpbx.com>;tag=1
To:<sip:10000@16692370001.zoomcloudpbx.com>
Max-Forwards: 7
Call-ID: 1-]]..user..[[@10.100.126.223
CSeq: 1 REGISTER
Contact: <sip:]]..user..[[@10.100.125.18:9090;transport=TCP>
Content-Length: 0
Expires: 86400]].."\r\n\r\n"
        log("===>"..reg)
        local resp = sendmsg(reg)
        log("--->"..tostring(resp))
        local uri = "sip:16692370001.zoomcloudpbx.com;transport=TCP"
        local nonce = tostring(string.match( resp, 'nonce="(.*)",'))
        local realm = tostring(string.match( resp, 'realm="(.*)", non'))
        local Response=get_auth(uri, password, method, user, nonce, realm)
        reg = [[REGISTER sip:16692370001.zoomcloudpbx.com;transport=TCP SIP/2.0
Via: SIP/2.0/TCP 10.100.125.18:9090; branch=z9hG4bK-3899-1-2;rport
From: <sip:]]..user..[[@16692370001.zoomcloudpbx.com>;tag=1
To: <sip:10000@16692370001.zoomcloudpbx.com>
Max-Forwards: 7
Call-ID: 1-]]..user..[[@10.100.126.223
CSeq: 3 REGISTER
Contact: <sip:]]..user..[[@10.100.125.18:9090;transport=TCP>
Authorization: Digest username="]]..user..[[",realm="]]..realm..[[",cnonce="6b8b4567",nc=00000001,qop=auth,uri="]]..uri..[[",nonce="]]..nonce..[[",response="]]..Response..[[",algorithm=MD5
Content-Length: 0
Expires: 86400]].."\r\n\r\n"
        log("===>"..reg)
        resp = sendmsg(reg)
        log("===rereg response "..tostring(resp))
        sleep(3)
    end
end

start_worker(worker)