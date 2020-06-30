while true do
     local resp = sip.sendmsg("\r\n\r\n")
     sip.log("  recieved ".. tostring(resp))
     sip.sleep(1)
     --sip.log("=============================>continue ")
end
