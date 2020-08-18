while true do
     local resp = sip.sendmsg("\r\n\r\n")
     log("  recieved ".. tostring(resp))
     sip.sleep(3)
     --sip.log("=============================>continue ")
end
