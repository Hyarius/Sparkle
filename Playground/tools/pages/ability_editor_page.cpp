#include "tools/pages/ability_editor_page.hpp"

#include "tools/widgets/atlas_cell_picker.hpp"
#include "tools/widgets/id_picker.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace
{
	[[nodiscard]] bool validId(const std::string &p_id)
	{
		return !p_id.empty() && std::ranges::all_of(p_id, [](unsigned char c) {
			return std::islower(c) || std::isdigit(c) || c == '-';
		});
	}
	[[nodiscard]] std::string join(const nlohmann::json &p_values)
	{
		std::string result;
		for (const auto &v : p_values)
		{
			if (!result.empty())
			{
				result += ", ";
			}
			if (v.is_string())
			{
				result += v.get<std::string>();
			}
		}
		return result;
	}
	[[nodiscard]] std::vector<std::string> split(const std::string &p_text)
	{
		std::vector<std::string> result;
		std::stringstream stream(p_text);
		std::string value;
		while (std::getline(stream, value, ','))
		{
			const auto first = value.find_first_not_of(" \t");
			const auto last = value.find_last_not_of(" \t");
			if (first != std::string::npos)
			{
				result.push_back(value.substr(first, last - first + 1));
			}
		}
		return result;
	}
}

namespace pg::tools
{
	AbilityEditorPage::AbilityEditorPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services) :
		IEditorPage(p_name, p_parent),
		_services(p_services),
		_background(p_name + "/Background", this),
		_leftPane(p_name + "/Left", this),
		_domainButton(p_name + "/Domain", "Domain: Abilities", &_leftPane),
		_newButton(p_name + "/New", "New", &_leftPane),
		_saveButton(p_name + "/Save", "Save", &_leftPane),
		_properties(p_name + "/Properties", this),
		_rulesPreview(p_name + "/RulesPreview", this),
		_icons(std::make_shared<spk::SpriteSheet>())
	{
		_icons->loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "spriteSheet.png", {8u, 6u});
		_background.activate();
		_leftPane.activate();
		_domainButton.activate();
		_newButton.activate();
		_saveButton.activate();
		_properties.activate();
		_rulesPreview.activate();
		_domainButton.subscribeToClick([this]() {
						 _toggleDomain();
					 })
			.relinquish();
		_newButton.subscribeToClick([this]() {
					  _newDocument();
				  })
			.relinquish();
		_saveButton.subscribeToClick([this]() {
					   save();
				   })
			.relinquish();
		_rulesPreview.setMinimalWidth(260);
		_rulesPreview.setLinePadding(4);
		_mainLayout.setElementPadding({6, 6});
		_mainLayout.addWidget(&_leftPane, spk::Layout::SizePolicy::Fixed)->setSize({230, 0});
		_mainLayout.addWidget(&_properties);
		_mainLayout.addWidget(&_rulesPreview, spk::Layout::SizePolicy::Fixed)->setSize({330, 0});
		_leftLayout.setElementPadding({4, 4});
		reload();
	}

	void AbilityEditorPage::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_mainLayout.setGeometry(geometry().atOrigin().shrink({6, 6}));
		_leftLayout.setGeometry(_leftPane.geometry().atOrigin());
	}

	void AbilityEditorPage::_rebuildList()
	{
		_leftLayout.clear();
		_documentButtons.clear();
		_domainButton.setText(_domain == ContentDocument::Domain::Ability ? "Domain: Abilities" : "Domain: Statuses");
		_leftLayout.addWidget(&_domainButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_newButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_saveButton, spk::Layout::SizePolicy::Minimum);
		const auto directory = _services.dataDirectory() / ContentDocument::directoryName(_domain);
		std::vector<std::filesystem::path> files;
		for (const auto &entry : std::filesystem::directory_iterator(directory))
		{
			if (entry.path().extension() == ".json")
			{
				files.push_back(entry.path());
			}
		}
		std::ranges::sort(files);
		for (const auto &file : files)
		{
			auto button = std::make_unique<spk::PushButton>(name() + "/Doc/" + file.stem().string(), file.stem().string(), &_leftPane);
			button->subscribeToClick([this, file]() {
					  _selectDocument(file, _domain);
				  })
				.relinquish();
			button->activate();
			_leftLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
			_documentButtons.push_back(std::move(button));
		}
		_onGeometryChange();
	}

	void AbilityEditorPage::_toggleDomain()
	{
		if (_document.dirty())
		{
			_services.report("Save or reload before switching domains");
			return;
		}
		_domain = _domain == ContentDocument::Domain::Ability ? ContentDocument::Domain::Status : ContentDocument::Domain::Ability;
		const auto ids = _domain == ContentDocument::Domain::Ability ? _services.registries().abilities().ids() : _services.registries().statuses().ids();
		if (!ids.empty())
		{
			_selectDocument(_services.dataDirectory() / ContentDocument::directoryName(_domain) / (ids.front() + ".json"), _domain);
		}
	}

	void AbilityEditorPage::_selectDocument(const std::filesystem::path &p_file, ContentDocument::Domain p_domain)
	{
		if (_document.dirty())
		{
			_services.report("Save or reload before switching documents");
			return;
		}
		_domain = p_domain;
		_document.load(p_file, p_domain);
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_refreshPreview();
	}

	void AbilityEditorPage::_newDocument()
	{
		if (_document.dirty())
		{
			_services.report("Save or reload before creating content");
			return;
		}
		const std::string base = _domain == ContentDocument::Domain::Ability ? "new-ability" : "new-status";
		std::string id = base;
		const auto directory = _services.dataDirectory() / ContentDocument::directoryName(_domain);
		for (int suffix = 2; std::filesystem::exists(directory / (id + ".json")); ++suffix)
		{
			id = base + "-" + std::to_string(suffix);
		}
		_document.create(_services.dataDirectory(), id, _domain, _services.registries());
		_state.markChanged();
		_rebuildProperties();
		_refreshPreview();
	}

	void AbilityEditorPage::_changed(bool p_rebuildProperties)
	{
		(void)_document.editJson();
		_state.markChanged();
		if (p_rebuildProperties)
		{
			_rebuildProperties();
		}
		_refreshPreview();
	}

	void AbilityEditorPage::_addJsonFields(nlohmann::json &p_object, const std::string &p_prefix)
	{
		if (p_object.contains("type") && p_object["type"].is_string())
		{
			const std::string type = p_object["type"].get<std::string>();
			if (type == "turnBased" || type == "seconds" || type == "infinite")
			{
				_properties.addEnum(p_prefix + " type", {"turnBased", "seconds", "infinite"}, type, [this, &p_object](const std::string &p_type) {
					if (p_type == "turnBased")
					{
						p_object = {{"type", p_type}, {"turns", 1}};
					}
					else if (p_type == "seconds")
					{
						p_object = {{"type", p_type}, {"seconds", 1.0}};
					}
					else
					{
						p_object = {{"type", p_type}};
					}
					_changed(true);
				});
			}
		}
		std::vector<std::string> keys;
		for (const auto &[key, unused] : p_object.items())
		{
			(void)unused;
			if (key != "type")
			{
				keys.push_back(key);
			}
		}
		for (const std::string &key : keys)
		{
			nlohmann::json &value = p_object[key];
			const std::string label = p_prefix + " " + key;
			if (value.is_object())
			{
				_addJsonFields(value, label);
				continue;
			}
			if (key == "status")
			{
				auto picker = std::make_unique<IdPicker>(name() + "/" + label, &_properties);
				picker->setIds(_services.registries().statuses().ids());
				picker->select(value.get<std::string>());
				picker->subscribeToSelection([this, &value](const std::string &p_id) {
					value = p_id;
					_changed();
				});
				_properties.addCustom(label, std::move(picker));
				continue;
			}
			if (value.is_boolean())
			{
				_properties.addBool(label, value.get<bool>(), [this, &value](bool p) {
					value = p;
					_changed();
				});
			}
			else if (value.is_number_integer())
			{
				_properties.addInt(label, value.get<int>(), -100000, 100000, [this, &value](int p) {
					value = p;
					_changed();
				});
			}
			else if (value.is_number_float())
			{
				_properties.addFloat(label, value.get<float>(), -10000, 10000, [this, &value](float p) {
					value = p;
					_changed();
				});
			}
			else if (value.is_array() && std::ranges::all_of(value, [](const auto &entry) {
						 return entry.is_string();
					 }))
			{
				_properties.addString(label, join(value), [this, &value](const std::string &p) {
					value = split(p);
					_changed();
				});
			}
			else if (value.is_string())
			{
				std::vector<std::string> options;
				if (key == "kind" && p_prefix.starts_with("Travel"))
				{
					options = {"projectile", "beam"};
				}
				else if (key == "damageKind" || key == "kind")
				{
					options = {"physical", "magical"};
				}
				else if (key == "resource")
				{
					options = {"health", "ap", "mp", "range", "turnBarTime"};
				}
				else if (key == "orientation")
				{
					options = {"towardCaster", "awayFromCaster"};
				}
				if (!options.empty() && key == "kind" && p_prefix.starts_with("Travel"))
				{
					_properties.addEnum(label, std::move(options), value.get<std::string>(), [this, &p_object](const std::string &p) {
						const std::string vfx = p_object.value("vfx", "vfx");
						p_object = p == "projectile" ? nlohmann::json{{"kind", p}, {"vfx", vfx}, {"speed", 10.0}}
													 : nlohmann::json{{"kind", p}, {"vfx", vfx}, {"duration", 0.5}};
						_changed(true);
					});
				}
				else if (!options.empty())
				{
					_properties.addEnum(label, std::move(options), value.get<std::string>(), [this, &value](const std::string &p) {
						value = p;
						_changed();
					});
				}
				else
				{
					_properties.addString(label, value.get<std::string>(), [this, &value](const std::string &p) {
						value = p;
						_changed();
					});
				}
			}
		}
	}

	void AbilityEditorPage::_rebuildProperties()
	{
		_properties.clear();
		if (_document.id().empty())
		{
			return;
		}
		nlohmann::json &root = _document.data();
		_properties.addString("Id", _document.id(), [this](const std::string &p) {
			if (validId(p))
			{
				_document.rename(p);
				_changed();
			}
		});
		_properties.addString("Display Name", root.value("displayName", ""), [this, &root](const std::string &p) {
			root["displayName"] = p;
			_changed();
		});
		const auto icon = root.value("icon", nlohmann::json::array({0, 0}));
		auto picker = std::make_unique<AtlasCellPicker>(name() + "/Icon", _icons, &_properties);
		picker->setCell({icon[0].get<int>(), icon[1].get<int>()});
		picker->subscribeToSelection([this, &root](const AtlasCell &p) {
			root["icon"] = {p.column, p.row};
			_changed();
		});
		_properties.addCustom("Icon", std::move(picker));
		if (_domain == ContentDocument::Domain::Ability)
		{
			_addJsonFields(root["cost"], "Cost");
			_addJsonFields(root["range"], "Range");
			if (root.contains("areaOfEffect"))
			{
				_addJsonFields(root["areaOfEffect"], "Area");
			}
			_properties.addEnum("Target", {"everything", "ally", "enemy", "empty"}, root.value("targetProfile", "enemy"), [this, &root](const std::string &p) {
				root["targetProfile"] = p;
				_changed();
			});
			_properties.addString("Caster Animation", root.value("casterAnimation", ""), [this, &root](const std::string &p) {
				if (p.empty())
				{
					root.erase("casterAnimation");
				}
				else
				{
					root["casterAnimation"] = p;
				}
				_changed();
			});
			_properties.addString("Target Animation", root.value("targetAnimation", "TakeDamage"), [this, &root](const std::string &p) {
				root["targetAnimation"] = p;
				_changed();
			});
			auto travel = std::make_unique<spk::PushButton>(name() + "/Travel", root.contains("travelVfx") ? "Remove Travel VFX" : "Add Travel VFX", &_properties);
			travel->subscribeToClick([this, &root]() {
					  if (root.contains("travelVfx"))
					  {
						  root.erase("travelVfx");
					  }
					  else
					  {
						  root["travelVfx"] = {{"kind", "projectile"}, {"vfx", "projectile"}, {"speed", 10.0}};
					  }
					  _changed(true);
				  })
				.relinquish();
			_properties.addCustom("Travel VFX", std::move(travel));
			if (root.contains("travelVfx"))
			{
				_addJsonFields(root["travelVfx"], "Travel");
			}
		}
		else
		{
			_properties.addString("Tags", join(root["tags"]), [this, &root](const std::string &p) {
				root["tags"] = split(p);
				_changed();
			});
			_properties.addEnum("Hook", {"turnStart", "turnEnd", "beforeConsumingResources", "afterConsumingResources", "onHPLoss", "onAPLoss", "onMPLoss", "onHPGain", "onAPGain", "onMPGain"}, root.value("hookPoint", "turnStart"), [this, &root](const std::string &p) {
				root["hookPoint"] = p;
				_changed();
			});
		}
		auto add = std::make_unique<spk::PushButton>(name() + "/AddEffect", "Add Effect", &_properties);
		add->subscribeToClick([this, &root]() {
			   root["effects"].push_back(ContentDocument::effectDefaults("damage"));
			   _changed(true);
		   })
			.relinquish();
		_properties.addCustom("Effects", std::move(add));
		for (std::size_t index = 0; index < root["effects"].size(); ++index)
		{
			nlohmann::json &effect = root["effects"][index];
			_properties.addEnum("Effect Type", ContentDocument::effectTypes(), effect.value("type", "damage"), [this, &root, index](const std::string &p) {
				root["effects"][index] = ContentDocument::effectDefaults(p);
				_changed(true);
			});
			_addJsonFields(effect, "Effect");
			auto remove = std::make_unique<spk::PushButton>(name() + "/RemoveEffect/" + std::to_string(index), "Remove", &_properties);
			remove->subscribeToClick([this, &root, index]() {
					  root["effects"].erase(root["effects"].begin() + static_cast<std::ptrdiff_t>(index));
					  _changed(true);
				  })
				.relinquish();
			_properties.addCustom("Effect", std::move(remove));
		}
	}

	void AbilityEditorPage::_refreshPreview()
	{
		_rulesPreview.setText(_document.rulesPreview(_services.registries()));
	}
	std::string AbilityEditorPage::title() const
	{
		return "Ability / Status Editor";
	}
	bool AbilityEditorPage::hasUnsavedChanges() const
	{
		return _document.dirty();
	}
	void AbilityEditorPage::save()
	{
		const auto errors = _document.validate(_services.registries());
		if (!errors.empty())
		{
			_services.report("Cannot save: " + errors.front());
			return;
		}
		_document.save(_services.writer());
		const auto file = _document.file();
		std::string warning;
		try
		{
			_services.load();
		} catch (const std::exception &e)
		{
			warning = e.what();
		}
		_document.load(file, _domain);
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_refreshPreview();
		_services.report(warning.empty() ? "Saved content" : "Saved; registry reload pending: " + warning);
	}
	void AbilityEditorPage::reload()
	{
		auto file = _document.file();
		if (file.empty() || !std::filesystem::exists(file))
		{
			file = _services.dataDirectory() / "abilities" / "ember.json";
		}
		_domain = file.parent_path().filename() == "statuses" ? ContentDocument::Domain::Status : ContentDocument::Domain::Ability;
		_document.load(file, _domain);
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_refreshPreview();
	}
}
