#规定CMake的最低版本
cmake_minimum_required(VERSION 3.14)
#工程名称和语言
Project(4G_Text LANGUAGES C)
#生成可执行文件
add_executable(mytext text.c)
#添加子目录 子目录中创建的目录是全局可见的 子目录中的相对路径就是相对于子目录自身
add_subdirectory(EG800AK)
add_subdirectory(Window_serial)
#把子目录中的静态库目标链接到可执行文件目标 
target_link_libraries(mytext PRIVATE es800ka)
target_link_libraries(mytext PRIVATE window_serial)