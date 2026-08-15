import re

with open("interface/include/bharat/uapi/personality/personality.h", "r") as f:
    content = f.read()

# Replace enum values with BH_ prefix because BHARAT_PERSONALITY_LINUX is a #define from cmake
content = content.replace("BHARAT_PERSONALITY_NATIVE", "BH_PERSONALITY_NATIVE")
content = content.replace("BHARAT_PERSONALITY_LINUX", "BH_PERSONALITY_LINUX")
content = content.replace("BHARAT_PERSONALITY_ANDROID", "BH_PERSONALITY_ANDROID")
content = content.replace("BHARAT_PERSONALITY_WINDOWS", "BH_PERSONALITY_WINDOWS")
content = content.replace("BHARAT_PERSONALITY_POSIX_LITE", "BH_PERSONALITY_POSIX_LITE")
content = content.replace("BHARAT_PERSONALITY_AUTOMOTIVE", "BH_PERSONALITY_AUTOMOTIVE")
content = content.replace("BHARAT_PERSONALITY_ROBOTICS", "BH_PERSONALITY_ROBOTICS")
content = content.replace("BHARAT_PERSONALITY_MAX", "BH_PERSONALITY_MAX")

with open("interface/include/bharat/uapi/personality/personality.h", "w") as f:
    f.write(content)
