require "util"
function run()
     while true do
          local resp = sendmsg("\r\n\r\n")
          log("  recieved ".. tostring(resp))
          sleep(3)
     end
end
start_worker(run)