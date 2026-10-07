import re

# Patch GeneratedCityLoader
with open('backend/src/model/GeneratedCityLoader.cpp', 'r') as f:
    gen_content = f.read()

gen_content = gen_content.replace('return network;', 'network.buildIndex();\n    return network;')
with open('backend/src/model/GeneratedCityLoader.cpp', 'w') as f:
    f.write(gen_content)

# Patch MontrealOSMLoader
with open('backend/src/model/MontrealOSMLoader.cpp', 'r') as f:
    mtl_content = f.read()

mtl_content = mtl_content.replace('network = std::move(handler.network);', 'handler.network.buildIndex();\n        network = std::move(handler.network);')
with open('backend/src/model/MontrealOSMLoader.cpp', 'w') as f:
    f.write(mtl_content)

