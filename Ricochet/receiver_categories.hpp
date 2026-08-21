#pragma once
enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 <<0,
	kLocalPlayer = 1 << 1,
	kForeignPlayer = 1 << 2,

};