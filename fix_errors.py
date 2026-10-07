import re

with open('backend/src/api/HttpServer.cpp', 'r') as f:
    content = f.read()

# Replace json{{"error", "..."}} with json{{"error", {{"code", "BAD_REQUEST"}, {"message", "..."}}}}
def replace_inline(m):
    msg = m.group(1)
    # Check if status 500 should be used? Usually these are 400.
    code = "BAD_REQUEST"
    if "Failed to load" in msg:
        code = "INTERNAL_ERROR"
    elif "not found" in msg.lower() or "no route found" in msg.lower():
        code = "NOT_FOUND"
    
    return f'json{{"error", {{"code", "{code}"}}, {{"message", "{msg}"}}}}'

# In C++, json{{"a", "b"}, {"c", "d"}} is valid. 
# But the nested json is: json{{"error", {{"code", "BAD_REQUEST"}, {"message", "..."}}}}

def fix_err(m):
    msg = m.group(1)
    return f'json err = {{"error", {{"code", "BAD_REQUEST"}}, {{"message", "{msg}"}}}};'

# Actually wait, nlohmann::json uses json{{"key", {{"inner", "val"}, {"inner2", "val2"}}}}
def replacer(m):
    msg = m.group(1)
    code = "BAD_REQUEST"
    if "Failed to load" in msg: code = "INTERNAL_ERROR"
    elif "not found" in msg.lower() or "no route found" in msg.lower(): code = "NOT_FOUND"
    return f'json{{{{"error", {{{{"code", "{code}"}}, {{"message", "{msg}"}}}}}}}}'

content = re.sub(r'json\{\{"error",\s*"([^"]+)"\}\}', replacer, content)

def replacer_err(m):
    msg = m.group(1)
    return f'json err = {{{{"error", {{{{"code", "BAD_REQUEST"}}, {{"message", "{msg}"}}}}}}}};';

content = re.sub(r'json err = \{\{"error",\s*"([^"]+)"\}\};', replacer_err, content)

with open('backend/src/api/HttpServer.cpp', 'w') as f:
    f.write(content)
