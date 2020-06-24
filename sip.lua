function get_msg()
    if SIP_MSG then
        return SIP_MSG
    else
        return nil
    end
end
while true do
    if get_msg() then
        sip.log("get msg "..SIP_MSG)
        sip.sleep(3)
        sip.sendmsg("\r\n\r\n")
    else
        sip.log(" lua send ping ")
        sip.sendmsg("\r\n\r\n")
        sip.sleep(3)
    end
    coroutine.yield()
end
