log("--------------sip script run------------")
while true done
    if SIP_MSG then
        log("==========get msg "..SIP_MSG)
    else
        --first time
        log("=============== send ping msg")
        sendmsg("\r\n\r\n")
    end
end
