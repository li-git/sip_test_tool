while true do
    sip.log(" lua send ping ")
    local resp = sip.sendmsg("\r\n\r\n")
    sip.log("=====================response "..tostring(resp))
    sip.sleep(3)
end
