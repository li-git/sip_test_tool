while true do
     local resp = tt.sendmsg("\r\n\r\n")
     log("  recieved ".. tostring(resp))
     tt.sleep(3)
     --tt.log("=============================>continue ")
end
