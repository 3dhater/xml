# xml

Read or write XML file with only 1 header file.

One XPath command to select nodes

	kkXMLDocument xml;
	xml.Read(u"E:/game.vcxproj");
	xml.Print();
	auto nodes = xml.SelectNodes(std::u16string(u"/Project/ItemGroup"));
