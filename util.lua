--local mysqlret = mysql("root","Pass_123", "10.100.125.17", 3306, "test", "select * from siptool")

RUN_STATE = "WAKE_START"

function get_auth(uri, password, method, user, nonce, realm)
    local HASH1=tt.md5(user..":"..realm..":"..password) 
    local HASH2=tt.md5(method..":"..uri)
    local auth = "auth"
    local nc = "00000001"
    local cnonce = "6b8b4567"
    local authstring = HASH1..":"..nonce..":"..nc..":"..cnonce..":"..auth..":"..HASH2
    return tt.md5(authstring)
end
function sendmsg(msg)
    tt.sendmsg(msg)
    RUN_STATE = "WAKE_DATA"
    return  coroutine.yield()
end
function sleep(delay)
    tt.sleep(delay)
    RUN_STATE = "WAKE_TIMER"
    return coroutine.yield()
end
function start_worker(worker_fun)
    worker_co = coroutine.create(worker_fun)
end
function luarunFun(wake_type, data)
    --local args = select("#",...)
    --local str = select(1,...)
    if wake_type ~= RUN_STATE then
        return
    else
        RUN_STATE = "WAKE_START"
        coroutine.resume(worker_co, data)
    end
end