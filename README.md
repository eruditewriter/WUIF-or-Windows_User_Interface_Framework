# WUIF
Windows User Interface Framework (WUIF) - C++ based framework for creating a universal GUI framework that utilizes Direct3D11/12, Direct2D, and DirectWrite.

This is an ongoing project to create a similar framework (but not a true replication) of WPF but for C++, utilizing best practices. This code accomodates automatic detection of Windows OS version (down to the particular edition, i.e. Win 10 Anniversary vs Win 10 Creators Update) and accomodates the unique properties of that OS (for example not starting an applicaiton that requires DirectX 12).

Currently the following is implemented:

OS Detection
D3D11 piplelines
D2D pipelines
DXGI Fullscreen toggle switching (this took a bit of research and testing to gt working properly)
Fullscreen windowed mode (vs above DXGI fullscreen)
Usual Windows winow aspects

More to come
