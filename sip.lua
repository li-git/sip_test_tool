--while true do
    --local resp = sip.sendmsg("\r\n\r\n")
    local reg = [[REGISTER sip:16692370001.zoomcloudpbx.com;transport=TCP SIP/2.0
Via: SIP/2.0/TCP 10.100.125.18:9090;branch=z9hG4bK-3899-1-0;rport
From:<sip:10000@10.100.125.18>;tag=1
To:<sip:10000@16692370001.zoomcloudpbx.com>
Max-Forwards: 7
Call-ID: 1-3899@10.100.126.223
CSeq: 1 REGISTER
Contact: <sip:10000@10.100.125.18:9090;transport=TCP>
Content-Length: 0
Expires: 86400]].."\r\n\r\n"

    sip.log(reg)
    local resp = sip.sendmsg(reg)
    sip.log("===response "..tostring(resp).."\r\n\r\n")

    local uri = "sip:10.100.125.116:5060"
    local password = "1234"
    local method = "REGISTER"
    local username = "10000"
    local nonce = tostring(string.match( resp, 'nonce="(.*)",'))
    local realm = tostring(string.match( resp, 'realm="(.*)", non'))

    local HASH1=sip.md5(username..":"..realm..":"..password) 
    local HASH2=sip.md5(method..":"..uri) 
    local Response=sip.md5(HASH1..":"..nonce..":"..HASH2)

    reg = [[REGISTER sip:16692370001.zoomcloudpbx.com;transport=TCP SIP/2.0
Via: SIP/2.0/TCP 10.100.126.223:9090; branch=z9hG4bK-3899-1-2;rport
From: <sip:10000@10.100.126.241>;tag=1
To: <sip:10000@16692370001.zoomcloudpbx.com>
Max-Forwards: 7
Call-ID: 1-3899@10.100.126.223
CSeq: 3 REGISTER
Contact: <sip:10000@10.100.126.223:9090;transport=TCP>
Authorization: Digest username="10000",realm="10.100.126.241",cnonce="6b8b4567",nc=00000001,qop=auth,uri="sip:10.100.125.116:5060",nonce="]]..nonce..[[",response="]]..Response..[[",algorithm=MD5
Content-Length: 0
Expires: 86400]].."\r\n\r\n"

    sip.log(reg)
    resp = sip.sendmsg(reg)
    sip.log("===rereg response "..tostring(resp))

    sip.sleep(1)

    sip.log("=============================>test ")


--end
